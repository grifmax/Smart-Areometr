#include <Arduino.h>
#include "SerialCompat.h"
#include "config.h"

#include "CapacitiveSensor.h"
#include "TemperatureCompensation.h"
#include "DisplayManager.h"
#include "WebServer.h"
#include "DataLogger.h"

// Глобальные объекты
CapacitiveSensor capacitiveSensor(CAPACITIVE_SENSOR_PIN, MEASUREMENT_SAMPLES, MEASUREMENT_DELAY);
TemperatureCompensation tempCompensation(TEMP_SENSOR_PIN);
DisplayManager display;
WebServerManager webServer;
DataLogger logger;

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

    // Инициализация Wi-Fi и веб-сервера
    display.showMessage("Starting Wi-Fi...", 1000);

    // Попытка подключения к сохраненной сети, иначе создаем AP
    // В реальном проекте здесь нужно загрузить SSID/пароль из конфигурации
    if (!webServer.beginAP()) {
        display.showError("WiFi failed");
        delay(2000);
    }

    // Настройка callback'ов для веб-сервера
    webServer.setAlcoholCallback([]() { return currentAlcohol; });
    webServer.setTemperatureCallback([]() { return currentTemperature; });
    webServer.setCalibratedCallback([]() { return isCalibrated; });

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
    // Обработка OTA обновлений
    webServer.handleOTA();

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

    // Отображение результатов
    display.showMeasurement(currentAlcohol, currentTemperature, true);

    Serial.println("=== Measurement ===");
    Serial.println("Raw Alcohol: " + String(rawAlcohol) + "%");
    Serial.println("Compensated: " + String(currentAlcohol) + "%");
    Serial.println("Temperature: " + String(currentTemperature) + "°C");
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
