#include <Arduino.h>
#include "SerialCompat.h"
#include "config.h"

#include "CapacitiveSensor.h"
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

// Глобальные объекты
CapacitiveSensor capacitiveSensor(CAPACITIVE_SENSOR_PIN, MEASUREMENT_SAMPLES, MEASUREMENT_DELAY);
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

    // Инициализация емкостного датчика
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
    mqttBridge.setStabilityCallback([]() { return 0; });  // TODO: добавить расчет стабильности
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

    // Callbacks для фракций
    webServer.setGetFractionStatusCallback([]() {
        JsonDocument doc;
        doc["current_fraction"] = FractionDetector::getFractionName(fractionDetector.getCurrentFraction());
        doc["fraction_color"] = FractionDetector::getFractionColor(fractionDetector.getCurrentFraction());
        doc["alcohol_rate"] = 0.0f;  // TODO: добавить расчет скорости
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
        doc["enabled"] = true;  // TODO: получить из настроек
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

    // Измерение процента алкоголя
    float rawAlcohol = capacitiveSensor.measureAlcoholPercent();

    // Температурная компенсация
    currentAlcohol = tempCompensation.compensate(rawAlcohol);

    // Обновление детектора фракций
    Fraction currentFraction = fractionDetector.update(currentAlcohol, currentTemperature);

    // Логирование в сессию
    if (session.getState() == SessionState::RUNNING) {
        uint8_t stability = 0;  // TODO: добавить расчет стабильности
        session.logMeasurement(currentAlcohol, currentTemperature, stability, currentFraction);
    }

    // Публикация в MQTT
    if (mqttBridge.isConnected()) {
        uint8_t stability = 0;  // TODO: добавить расчет стабильности
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

    uint16_t rawValue = capacitiveSensor.readRaw();
    display.showCalibration(calibrationStep, rawValue);

    // Проверка нажатия кнопки для подтверждения
    bool buttonState = digitalRead(BUTTON_MEASURE);
    if (buttonState == LOW && !buttonPressed) {
        buttonPressed = true;
        delay(50);  // Debounce

        if (calibrationStep == 0) {
            // Калибровка воды
            capacitiveSensor.calibrateWater();
            calibrationStep = 1;
            display.showMessage("Water calibrated!", 1500);
            Serial.println("Water calibration complete");

        } else if (calibrationStep == 1) {
            // Калибровка спирта
            capacitiveSensor.calibrateAlcohol();
            calibrationMode = false;
            calibrationStep = 0;
            isCalibrated = true;

            // Сохраняем калибровочные данные
            saveCalibrationData();

            display.showMessage("Calibration done!", 2000);
            Serial.println("Calibration complete!");
        }
    } else if (buttonState == HIGH) {
        buttonPressed = false;
    }
}

void loadCalibrationData() {
    // В реальном проекте здесь нужно загрузить данные из LittleFS
    // Пока используем заглушку
    Serial.println("Loading calibration data...");

    if (LittleFS.begin(true)) {
        if (LittleFS.exists("/calibration.json")) {
            File file = LittleFS.open("/calibration.json", "r");
            if (file) {
                JsonDocument doc;
                DeserializationError error = deserializeJson(doc, file);
                file.close();

                if (!error) {
                    float water = doc["water"];
                    float alcohol = doc["alcohol"];
                    capacitiveSensor.setCalibration(water, alcohol);
                    isCalibrated = true;
                    Serial.println("Calibration loaded: Water=" + String(water) + ", Alcohol=" + String(alcohol));
                }
            }
        }
    }
}

void saveCalibrationData() {
    Serial.println("Saving calibration data...");

    float water, alcohol;
    capacitiveSensor.getCalibration(water, alcohol);

    if (LittleFS.begin(true)) {
        File file = LittleFS.open("/calibration.json", "w");
        if (file) {
            JsonDocument doc;
            doc["water"] = water;
            doc["alcohol"] = alcohol;
            serializeJson(doc, file);
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
