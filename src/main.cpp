#include <Arduino.h>
#include "SerialCompat.h"
#include "config.h"

#include "CapacitiveSensorV2.h"
#include "TemperatureCompensation.h"
#include "DisplayManager.h"
#include "WebServer.h"
#include "DataLogger.h"
#include "MQTTBridge.h"
#include "FractionDetector.h"
#include "DistillationSession.h"
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <Preferences.h>

#ifdef USE_ADS1115
#include "ADS1115Driver.h"
#endif

// Глобальные объекты
#ifdef USE_ADS1115
ADS1115Driver ads1115Driver(ADS1115_I2C_ADDRESS);
CapacitiveSensorV2 capacitiveSensor(CAPACITIVE_SENSOR_PIN, MEASUREMENT_SAMPLES, MEASUREMENT_DELAY);
#else
CapacitiveSensorV2 capacitiveSensor(CAPACITIVE_SENSOR_PIN, MEASUREMENT_SAMPLES, MEASUREMENT_DELAY);
#endif
TemperatureCompensation tempCompensation(TEMP_SENSOR_PIN);
DisplayManager display;
WebServerManager webServer;
DataLogger logger;
MQTTBridge mqttBridge;
FractionDetector fractionDetector;
DistillationSession session;

// Переменные состояния
float currentAlcohol = 0.0f;
float currentTemperature = 20.0f;
bool isCalibrated = false;
bool calibrationMode = false;
uint8_t calibrationStep = 0;

// Таймеры
unsigned long lastMeasurement = 0;
unsigned long lastLogSave = 0;
const unsigned long measurementInterval = 5000;  // Измерения каждые 5 секунд
const unsigned long logInterval = 60000;         // Сохранение лога каждую минуту

// Прототипы функций
void performMeasurement();
void handleButtons();
void startCalibration();
void processCalibration();
void loadCalibrationData();
void saveCalibrationData();
void loadMQTTConfig();
void saveMQTTConfig();
void loadFractionThresholds();
void saveFractionThresholds();
void loadWiFiConfig();

void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println("\n\n=============================");
    Serial.println("Smart Areometr v" + String(FIRMWARE_VERSION));
    Serial.println("=============================\n");

    // Инициализация кнопок
    pinMode(BUTTON_CALIBRATE, INPUT_PULLUP);
    pinMode(BUTTON_MEASURE, INPUT_PULLUP);

    // Инициализация дисплея
    if (!display.begin()) {
        Serial.println("ERROR: Display initialization failed!");
    }
    display.showBootScreen();
    delay(2000);

#ifdef USE_ADS1115
    // Инициализация ADS1115 (16-битный внешний АЦП)
    Serial.println("Initializing ADS1115...");
    if (ads1115Driver.begin()) {
        Serial.println("ADS1115 initialized successfully");
        // Установка параметров
        // GAIN_FOUR определен в библиотеке Adafruit_ADS1X15
        ads1115Driver.setGain(GAIN_FOUR);  // Усиление 4×, диапазон ±1.024V
        ads1115Driver.setDataRate(ADS1115_DATA_RATE);
        
        // Тест подключения
        if (ads1115Driver.testConnection()) {
            Serial.println("ADS1115 connection test: PASSED");
        } else {
            Serial.println("WARNING: ADS1115 connection test failed");
        }
    } else {
        Serial.println("WARNING: ADS1115 initialization failed, using internal ADC");
    }
#endif

    // Инициализация емкостного датчика
#ifdef USE_ADS1115
    // Передаем драйвер ADS1115 в capacitiveSensor
    capacitiveSensor.setADS1115Driver(&ads1115Driver);
#endif
    capacitiveSensor.begin();

    // Инициализация температурного датчика
    if (!tempCompensation.begin()) {
        display.showError("Temp sensor failed");
        delay(2000);
    }

    // Загрузка калибровочных данных
    loadCalibrationData();

    // Инициализация логгера
    if (!logger.begin()) {
        Serial.println("WARNING: Logger initialization failed");
    }

    // Загрузка настроек фракций
    loadFractionThresholds();

    // Настройка callback при смене фракций
    fractionDetector.setFractionChangeCallback([](Fraction newFraction, Fraction oldFraction) {
        String fractionName = FractionDetector::getFractionName(newFraction);
        String oldFractionName = FractionDetector::getFractionName(oldFraction);
        
        Serial.printf("Fraction changed: %s -> %s\n", oldFractionName.c_str(), fractionName.c_str());
        
        // Публикация в MQTT
        if (mqttBridge.isConnected()) {
            mqttBridge.publishFractionChange(fractionName);
        }
    });

    // Загрузка настроек MQTT
    loadMQTTConfig();

    // Установка callbacks для MQTT
    mqttBridge.setAlcoholCallback([]() { return currentAlcohol; });
    mqttBridge.setTemperatureCallback([]() { return currentTemperature; });
    mqttBridge.setStabilityCallback([]() { return capacitiveSensor.getLastMeasurementStats().stability; });
    mqttBridge.setFractionCallback([]() { 
        return FractionDetector::getFractionName(fractionDetector.getCurrentFraction()); 
    });

    // Инициализация Wi-Fi и веб-сервера
    display.showMessage("Starting Wi-Fi...", 1000);

    // Попытка подключения к сохраненной сети, иначе создаем AP
    loadWiFiConfig();

    // Настройка callback'ов для веб-сервера
    webServer.setAlcoholCallback([]() { return currentAlcohol; });
    webServer.setTemperatureCallback([]() { return currentTemperature; });
    webServer.setCalibratedCallback([]() { return isCalibrated; });
    webServer.setRawValueCallback([]() { return capacitiveSensor.readRaw(); });
    webServer.setStabilityCallback([]() { return capacitiveSensor.getLastMeasurementStats().stability; });

    // Callbacks для калибровки
    webServer.setGetCalibrationDataCallback([]() {
        return capacitiveSensor.exportCalibration();
    });

    webServer.setAddCalibrationPointCallback([](float alcoholPercent, float temperature) {
        bool success = capacitiveSensor.addCalibrationPoint(alcoholPercent, temperature);
        if (success) {
            isCalibrated = capacitiveSensor.isCalibrated();
            saveCalibrationData();
        }
        return success;
    });

    webServer.setClearCalibrationCallback([]() {
        capacitiveSensor.clearCalibration();
        isCalibrated = false;
        saveCalibrationData();
    });

    // Для удаления точки нужно парсить JSON и пересоздавать калибровку
    webServer.setDeleteCalibrationPointCallback([](uint8_t index) {
        // Получаем текущую калибровку
        String json = capacitiveSensor.exportCalibration();
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, json);
        
        if (error) {
            return false;
        }

        JsonArray points = doc["points"].as<JsonArray>();
        if (index >= points.size()) {
            return false;
        }

        // Удаляем точку по индексу
        points.remove(index);

        // Очищаем калибровку и загружаем заново
        capacitiveSensor.clearCalibration();
        
        // Добавляем все точки кроме удаленной
        for (JsonObject point : points) {
            float alcohol = point["alcohol"] | 0.0f;
            float temp = point["temp"] | 20.0f;
            capacitiveSensor.addCalibrationPoint(alcohol, temp);
        }

        isCalibrated = capacitiveSensor.isCalibrated();
        saveCalibrationData();
        return true;
    });

    // Callbacks для фракций
    webServer.setGetFractionStatusCallback([]() {
        JsonDocument doc;
        doc["current_fraction"] = FractionDetector::getFractionName(fractionDetector.getCurrentFraction());
        doc["fraction_color"] = FractionDetector::getFractionColor(fractionDetector.getCurrentFraction());
        doc["alcohol_rate"] = fractionDetector.getAlcoholRate();
        doc["current_volume"] = fractionDetector.getCurrentVolume();
        doc["total_volume"] = fractionDetector.getTotalVolume();
        
        String json;
        serializeJson(doc, json);
        return json;
    });

    webServer.setGetFractionStatsCallback([]() {
        return fractionDetector.getStatsJSON();
    });

    webServer.setGetFractionThresholdsCallback([]() {
        return fractionDetector.saveSettings();
    });

    webServer.setSetFractionThresholdsCallback([](const String &json) {
        bool success = fractionDetector.loadSettings(json);
        if (success) {
            saveFractionThresholds();
        }
        return success;
    });

    webServer.setResetFractionSessionCallback([]() {
        fractionDetector.reset();
        Serial.println("Fraction session reset");
    });

    webServer.setGetFractionModeCallback([]() {
        JsonDocument doc;
        doc["mode"] = FractionDetector::getModeName(fractionDetector.getMode());
        String json;
        serializeJson(doc, json);
        return json;
    });

    webServer.setSetFractionModeCallback([](const String &json) {
        JsonDocument doc;
        if (deserializeJson(doc, json) != DeserializationError::Ok) {
            return false;
        }
        String modeStr = doc["mode"] | "mash";
        if (modeStr == "monitoring") {
            fractionDetector.setMode(DetectionMode::MONITORING_MODE);
        } else {
            fractionDetector.setMode(DetectionMode::MASH_MODE);
        }
        return true;
    });

    // Callbacks для MQTT
    webServer.setGetMQTTStatusCallback([]() {
        JsonDocument doc;
        doc["connected"] = mqttBridge.isConnected();
        doc["enabled"] = mqttBridge.isEnabled();
        String json;
        serializeJson(doc, json);
        return json;
    });

    webServer.setGetMQTTConfigCallback([]() {
        JsonDocument doc;
        // Загружаем из файла
        if (LittleFS.begin(false) && LittleFS.exists("/mqtt_config.json")) {
            File file = LittleFS.open("/mqtt_config.json", "r");
            if (file) {
                deserializeJson(doc, file);
                file.close();
            }
        }
        // Если файла нет, возвращаем дефолты
        if (doc.isNull()) {
            doc["enabled"] = false;
            doc["server"] = "";
            doc["port"] = 1883;
            doc["username"] = "";
            doc["password"] = "";
            doc["client_id"] = "smart-areometr";
            doc["base_topic"] = "distillery/areometer";
            doc["publish_interval"] = 5;
            doc["ha_discovery"] = true;
        }
        String json;
        serializeJson(doc, json);
        return json;
    });

    webServer.setSetMQTTConfigCallback([](const String &json) {
        JsonDocument doc;
        if (deserializeJson(doc, json) != DeserializationError::Ok) {
            return false;
        }

        bool enabled = doc["enabled"] | false;
        String server = doc["server"] | "";
        uint16_t port = doc["port"] | 1883;
        String username = doc["username"] | "";
        String password = doc["password"] | "";
        String clientId = doc["client_id"] | "smart-areometr";
        String baseTopic = doc["base_topic"] | "distillery/areometer";

        mqttBridge.setEnabled(enabled);
        mqttBridge.setClientId(clientId);
        mqttBridge.setBaseTopic(baseTopic);

        if (enabled && !server.isEmpty()) {
            mqttBridge.begin(server, port, username, password);
        }

        // Сохраняем в файл
        if (LittleFS.begin(false)) {
            File file = LittleFS.open("/mqtt_config.json", "w");
            if (file) {
                serializeJson(doc, file);
                file.close();
            }
        }

        return true;
    });

    webServer.setTestMQTTCallback([]() {
        return mqttBridge.isConnected();
    });

    // Callbacks для сессий
    webServer.setStartSessionCallback([](const String &name, float mashVol) {
        return session.start(name, mashVol);
    });

    webServer.setStopSessionCallback([]() {
        session.stop();
    });

    webServer.setPauseSessionCallback([]() {
        session.togglePause();
    });

    webServer.setGetSessionStatusCallback([]() {
        JsonDocument doc;
        doc["session_id"] = session.getSessionId();
        doc["state"] = (session.getState() == SessionState::RUNNING) ? "running" :
                       (session.getState() == SessionState::PAUSED) ? "paused" :
                       (session.getState() == SessionState::FINISHED) ? "finished" : "idle";
        doc["duration"] = session.getDuration();
        doc["duration_formatted"] = session.getDurationFormatted();
        doc["data_points"] = session.getDataPointsCount();
        doc["progress"] = session.getProgress();
        
        String json;
        serializeJson(doc, json);
        return json;
    });

    webServer.setExportSessionJSONCallback([]() {
        return session.exportToJSON();
    });

    webServer.setExportSessionCSVCallback([]() {
        return session.exportToCSV();
    });

    webServer.setGetSessionsListCallback([]() {
        return DistillationSession::getSessionsList();
    });

    // Показываем информацию о сети
    display.showNetworkInfo(webServer.getSSID(), webServer.getIP(), webServer.isConnected());
    delay(3000);

    Serial.println("\n=== System Ready ===");
    Serial.println("IP Address: " + webServer.getIP());
    Serial.println("Open browser and navigate to: http://" + webServer.getIP());
    Serial.println("====================\n");

    lastMeasurement = millis();
}

void loop() {
    // Обработка веб-сервера и OTA обновлений
    webServer.handle();

    // Обработка MQTT
    mqttBridge.handle();

    // Обработка кнопок
    handleButtons();

    // Режим калибровки
    if (calibrationMode) {
        processCalibration();
        return;  // В режиме калибровки не выполняем измерения
    }

    // Периодические измерения
    unsigned long currentMillis = millis();
    if (currentMillis - lastMeasurement >= measurementInterval) {
        lastMeasurement = currentMillis;
        performMeasurement();
    }

    // Периодическое сохранение в лог
    if (currentMillis - lastLogSave >= logInterval && isCalibrated) {
        lastLogSave = currentMillis;
        logger.addMeasurement(currentAlcohol, currentTemperature, true);
    }

    // Обновление дисплея
    display.update();

    delay(10);
}

void performMeasurement() {
    if (!isCalibrated) {
        display.showError("Not calibrated!");
        Serial.println("ERROR: System not calibrated");
        return;
    }

    // Измерение температуры
    currentTemperature = tempCompensation.measureTemperature();
    
    // Измеряем крепость с температурной компенсацией (CapacitiveSensorV2 включает температурную компенсацию)
    float rawAlcohol = capacitiveSensor.measureAlcoholPercent(currentTemperature, true);
    
    // Если V2 вернул ошибку (датчик не откалиброван), показываем ошибку
    if (rawAlcohol < 0) {
        display.showError("Sensor error!");
        Serial.println("ERROR: Failed to measure alcohol percent");
        return;
    }
    
    currentAlcohol = rawAlcohol;

    // Обновление детектора фракций
    Fraction currentFraction = fractionDetector.update(currentAlcohol, currentTemperature);

    // Получаем стабильность сигнала
    uint8_t stability = capacitiveSensor.getLastMeasurementStats().stability;

    // Логирование в сессию
    if (session.getState() == SessionState::RUNNING) {
        session.logMeasurement(currentAlcohol, currentTemperature, stability, currentFraction);
    }

    // Публикация в MQTT
    if (mqttBridge.isConnected()) {
        mqttBridge.publishMeasurement(currentAlcohol, currentTemperature, stability);
    }

    // Отображение результатов
    display.showMeasurement(currentAlcohol, currentTemperature, true);

    Serial.println("=== Measurement ===");
    Serial.println("Raw Alcohol: " + String(rawAlcohol) + "%");
    Serial.println("Compensated: " + String(currentAlcohol) + "%");
    Serial.println("Temperature: " + String(currentTemperature) + "°C");
    Serial.println("Fraction: " + FractionDetector::getFractionName(currentFraction));
    Serial.println("==================");
}

void handleButtons() {
    // Кнопка калибровки
    static bool lastCalibrateState = HIGH;
    bool currentCalibrateState = digitalRead(BUTTON_CALIBRATE);

    if (lastCalibrateState == HIGH && currentCalibrateState == LOW) {
        // Нажата кнопка калибровки
        delay(50);  // Debounce
        if (digitalRead(BUTTON_CALIBRATE) == LOW) {
            startCalibration();
        }
    }
    lastCalibrateState = currentCalibrateState;

    // Кнопка измерения
    static bool lastMeasureState = HIGH;
    bool currentMeasureState = digitalRead(BUTTON_MEASURE);

    if (lastMeasureState == HIGH && currentMeasureState == LOW) {
        // Нажата кнопка измерения
        delay(50);  // Debounce
        if (digitalRead(BUTTON_MEASURE) == LOW && isCalibrated) {
            performMeasurement();
        }
    }
    lastMeasureState = currentMeasureState;
}

void startCalibration() {
    Serial.println("Starting calibration process");
    calibrationMode = true;
    calibrationStep = 0;
    display.showMessage("Calibration mode", 1500);
}

void processCalibration() {
    static unsigned long lastUpdate = 0;
    static bool buttonPressed = false;

    if (millis() - lastUpdate < 500) {
        return;  // Обновляем экран каждые 500мс
    }
    lastUpdate = millis();

    // Измеряем температуру для калибровки
    float temp = tempCompensation.measureTemperature();
    if (temp < -50.0f || temp > 150.0f) {
        // Если температура невалидна, используем эталонную
        temp = TEMP_REFERENCE;
    }

    uint16_t rawValue = capacitiveSensor.readRaw();
    display.showCalibration(calibrationStep, rawValue);

    // Проверка нажатия кнопки для подтверждения
    bool buttonState = digitalRead(BUTTON_MEASURE);
    if (buttonState == LOW && !buttonPressed) {
        buttonPressed = true;
        delay(50);  // Debounce

        if (calibrationStep == 0) {
            // Калибровка воды (0% алкоголя)
            if (capacitiveSensor.addCalibrationPoint(0.0f, temp)) {
                calibrationStep = 1;
                display.showMessage("Water calibrated!", 1500);
                Serial.println("Water calibration complete at " + String(temp) + "°C");
            } else {
                display.showMessage("Calibration failed!", 1500);
                Serial.println("Water calibration failed");
            }

        } else if (calibrationStep == 1) {
            // Калибровка спирта (100% алкоголя)
            if (capacitiveSensor.addCalibrationPoint(100.0f, temp)) {
                calibrationMode = false;
                calibrationStep = 0;
                isCalibrated = capacitiveSensor.isCalibrated();

                // Сохраняем калибровочные данные
                saveCalibrationData();

                display.showMessage("Calibration done!", 2000);
                Serial.println("Calibration complete at " + String(temp) + "°C");
            } else {
                display.showMessage("Calibration failed!", 1500);
                Serial.println("Alcohol calibration failed");
            }
        }
    } else if (buttonState == HIGH) {
        buttonPressed = false;
    }
}

void loadCalibrationData() {
    Serial.println("Loading calibration data...");

    if (LittleFS.begin(true)) {
        if (LittleFS.exists("/calibration.json")) {
            File file = LittleFS.open("/calibration.json", "r");
            if (file) {
                String json = file.readString();
                file.close();

                // Пытаемся загрузить в новом формате (CapacitiveSensorV2)
                if (capacitiveSensor.importCalibration(json)) {
                    isCalibrated = capacitiveSensor.isCalibrated();
                    Serial.println("Calibration loaded (V2 format)");
                    return;
                }

                // Если не удалось, пробуем старый формат для обратной совместимости
                JsonDocument doc;
                DeserializationError error = deserializeJson(doc, json);
                if (!error && doc.containsKey("water") && doc.containsKey("alcohol")) {
                    // Конвертируем старый формат в новый
                    float waterRaw = doc["water"];
                    float alcoholRaw = doc["alcohol"];
                    float temp = doc["temperature"] | 20.0f;
                    
                    // Создаем JSON в новом формате
                    JsonDocument newDoc;
                    JsonArray pointsArray = newDoc["points"].to<JsonArray>();
                    
                    // Точка для воды (0%)
                    JsonObject waterPoint = pointsArray.add<JsonObject>();
                    waterPoint["alcohol"] = 0.0f;
                    waterPoint["raw"] = waterRaw;
                    waterPoint["temp"] = temp;
                    
                    // Точка для спирта (100%)
                    JsonObject alcoholPoint = pointsArray.add<JsonObject>();
                    alcoholPoint["alcohol"] = 100.0f;
                    alcoholPoint["raw"] = alcoholRaw;
                    alcoholPoint["temp"] = temp;
                    
                    String newJson;
                    serializeJson(newDoc, newJson);
                    
                    // Импортируем в новом формате
                    if (capacitiveSensor.importCalibration(newJson)) {
                        // Сохраняем в новом формате
                        saveCalibrationData();
                        
                        isCalibrated = capacitiveSensor.isCalibrated();
                        Serial.println("Calibration converted from old format: Water=" + String(waterRaw) + ", Alcohol=" + String(alcoholRaw));
                    } else {
                        Serial.println("Failed to convert old calibration format");
                    }
                }
            }
        }
    }
}

void saveCalibrationData() {
    Serial.println("Saving calibration data...");

    if (LittleFS.begin(true)) {
        File file = LittleFS.open("/calibration.json", "w");
        if (file) {
            String json = capacitiveSensor.exportCalibration();
            file.print(json);
            file.close();
            Serial.println("Calibration saved!");
        }
    }
}

void loadMQTTConfig() {
    if (!LittleFS.begin(false)) {
        Serial.println("MQTT: LittleFS not available, using defaults");
        return;
    }

    if (!LittleFS.exists("/mqtt_config.json")) {
        Serial.println("MQTT: Config file not found, using defaults");
        return;
    }

    File file = LittleFS.open("/mqtt_config.json", "r");
    if (!file) {
        Serial.println("MQTT: Failed to open config file");
        return;
    }

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, file);
    file.close();

    if (error) {
        Serial.println("MQTT: Failed to parse config JSON");
        return;
    }

    bool enabled = doc["enabled"] | false;
    String server = doc["server"] | "";
    uint16_t port = doc["port"] | 1883;
    String username = doc["username"] | "";
    String password = doc["password"] | "";
    String clientId = doc["client_id"] | "smart-areometr";
    String baseTopic = doc["base_topic"] | "distillery/areometer";

    mqttBridge.setEnabled(enabled);
    mqttBridge.setClientId(clientId);
    mqttBridge.setBaseTopic(baseTopic);

    if (enabled && !server.isEmpty()) {
        mqttBridge.begin(server, port, username, password);
        Serial.printf("MQTT: Config loaded - %s:%d\n", server.c_str(), port);
    } else {
        Serial.println("MQTT: Disabled or server not configured");
    }
}

void saveMQTTConfig() {
    if (!LittleFS.begin(false)) {
        Serial.println("MQTT: Failed to mount LittleFS for saving");
        return;
    }

    File file = LittleFS.open("/mqtt_config.json", "w");
    if (!file) {
        Serial.println("MQTT: Failed to open config file for writing");
        return;
    }

    JsonDocument doc;
    doc["enabled"] = mqttBridge.isConnected() || true;  // Сохраняем настройки даже если не подключено
    doc["server"] = "";  // Будет заполнено через API
    doc["port"] = 1883;
    doc["username"] = "";
    doc["password"] = "";
    doc["client_id"] = "smart-areometr";
    doc["base_topic"] = "distillery/areometer";
    doc["publish_interval"] = 5;
    doc["ha_discovery"] = true;

    serializeJson(doc, file);
    file.close();
    Serial.println("MQTT: Config saved");
}

void loadFractionThresholds() {
    if (!LittleFS.begin(false)) {
        Serial.println("FractionDetector: LittleFS not available, using defaults");
        return;
    }

    if (!LittleFS.exists("/fraction_thresholds.json")) {
        Serial.println("FractionDetector: Thresholds file not found, using defaults");
        return;
    }

    File file = LittleFS.open("/fraction_thresholds.json", "r");
    if (!file) {
        Serial.println("FractionDetector: Failed to open thresholds file");
        return;
    }

    String json = file.readString();
    file.close();

    if (fractionDetector.loadSettings(json)) {
        Serial.println("FractionDetector: Thresholds loaded");
    } else {
        Serial.println("FractionDetector: Failed to load thresholds");
    }
}

void saveFractionThresholds() {
    if (!LittleFS.begin(false)) {
        Serial.println("FractionDetector: Failed to mount LittleFS for saving");
        return;
    }

    File file = LittleFS.open("/fraction_thresholds.json", "w");
    if (!file) {
        Serial.println("FractionDetector: Failed to open thresholds file for writing");
        return;
    }

    String json = fractionDetector.saveSettings();
    file.print(json);
    file.close();
    Serial.println("FractionDetector: Thresholds saved");
}

void loadWiFiConfig() {
    Preferences preferences;
    preferences.begin("wifi", true);  // read-only
    
    String ssid = preferences.getString("ssid", "");
    String password = preferences.getString("password", "");
    
    preferences.end();
    
    // Если есть сохраненные настройки, пытаемся подключиться
    if (ssid.length() > 0) {
        Serial.println("Found saved WiFi config: " + ssid);
        if (webServer.beginClient(ssid, password)) {
            Serial.println("Connected to WiFi: " + ssid);
            display.showMessage("WiFi: " + ssid, 2000);
            return;
        } else {
            Serial.println("Failed to connect to saved WiFi, creating AP");
        }
    }
    
    // Если подключение не удалось или нет сохраненных настроек, создаем AP
    if (!webServer.beginAP()) {
        display.showError("WiFi failed");
        delay(2000);
    } else {
        display.showMessage("AP: " + String(DEFAULT_SSID), 2000);
    }
}
