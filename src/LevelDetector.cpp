#include <Arduino.h>
#include "SerialCompat.h"
#include "config.h"
#include "LevelDetector.h"
#include "ADS1115Driver.h"

LevelDetector::LevelDetector()
    : adsDriver(nullptr), enabled(RECEIVER_LEVEL_DETECTION_ENABLED),
      isOverflowing(false), lastOverflowState(false),
      thresholdVoltage(LEVEL_DETECTION_THRESHOLD),
      debounceMs(LEVEL_DETECTION_DEBOUNCE_MS),
      lastCheckTime(0), overflowStartTime(0),
      filterIndex(0), filterCount(0) {
    // Инициализация фильтра
    for (uint8_t i = 0; i < LEVEL_FILTER_SIZE; i++) {
        filterBuffer[i] = 0.0f;
    }
}

bool LevelDetector::begin(ADS1115Driver* driver) {
    if (!driver || !driver->isInitialized()) {
        Serial.println("LevelDetector: ERROR - ADS1115 driver not initialized!");
        return false;
    }
    
    adsDriver = driver;
    enabled = RECEIVER_LEVEL_DETECTION_ENABLED;
    
    Serial.print("LevelDetector: Initialized on channel AIN");
    Serial.print(ADS1115_LEVEL_CHANNEL);
    Serial.print(", threshold: ");
    Serial.print(thresholdVoltage, 3);
    Serial.println("V");
    
    return true;
}

void LevelDetector::update() {
    if (!enabled || !adsDriver || !adsDriver->isInitialized()) {
        return;
    }
    
    unsigned long now = millis();
    
    // Проверяем уровень с заданным интервалом
    if (now - lastCheckTime < LEVEL_CHECK_INTERVAL) {
        return;
    }
    
    lastCheckTime = now;
    
    // Получаем отфильтрованное значение напряжения
    float voltage = getFilteredVoltage();
    
    // Определяем состояние переполнения
    bool newOverflowState = (voltage >= thresholdVoltage);
    updateOverflowState(newOverflowState);
}

float LevelDetector::getFilteredVoltage() {
    if (!adsDriver || !adsDriver->isInitialized()) {
        return 0.0f;
    }
    
    // Читаем одиночный канал AIN2
    int16_t rawValue = adsDriver->readSingleEnded(ADS1115_LEVEL_CHANNEL);
    float voltage = adsDriver->getVoltage(rawValue);
    
    // Добавляем в фильтр скользящего среднего
    filterBuffer[filterIndex] = voltage;
    filterIndex = (filterIndex + 1) % LEVEL_FILTER_SIZE;
    
    if (filterCount < LEVEL_FILTER_SIZE) {
        filterCount++;
    }
    
    // Вычисляем среднее значение
    float sum = 0.0f;
    for (uint8_t i = 0; i < filterCount; i++) {
        sum += filterBuffer[i];
    }
    
    return sum / filterCount;
}

void LevelDetector::updateOverflowState(bool newState) {
    unsigned long now = millis();
    
    if (newState != lastOverflowState) {
        // Состояние изменилось - начинаем отсчет дебаунсинга
        if (newState) {
            // Переход в состояние переполнения
            overflowStartTime = now;
        } else {
            // Переход из состояния переполнения
            overflowStartTime = now;
        }
        lastOverflowState = newState;
    }
    
    // Проверяем дебаунсинг
    if (newState) {
        // Проверяем, достаточно ли времени прошло для подтверждения переполнения
        if (now - overflowStartTime >= debounceMs) {
            if (!isOverflowing) {
                // Переполнение только что обнаружено
                isOverflowing = true;
                Serial.println("LevelDetector: OVERFLOW DETECTED!");
                
                if (overflowCallback) {
                    overflowCallback();
                }
            }
        }
    } else {
        // Проверяем, достаточно ли времени прошло для подтверждения очистки
        if (now - overflowStartTime >= debounceMs) {
            if (isOverflowing) {
                // Переполнение очищено
                isOverflowing = false;
                Serial.println("LevelDetector: Overflow cleared");
                
                if (clearCallback) {
                    clearCallback();
                }
            }
        }
    }
}

float LevelDetector::getCurrentVoltage() {
    return getFilteredVoltage();
}

void LevelDetector::setThreshold(float voltage) {
    if (voltage < 0.0f || voltage > 5.0f) {
        Serial.println("LevelDetector: WARNING - Invalid threshold voltage!");
        return;
    }
    
    thresholdVoltage = voltage;
    Serial.print("LevelDetector: Threshold set to ");
    Serial.print(thresholdVoltage, 3);
    Serial.println("V");
}

float LevelDetector::getRawVoltage() const {
    if (!adsDriver || !adsDriver->isInitialized()) {
        return 0.0f;
    }
    
    int16_t rawValue = adsDriver->readSingleEnded(ADS1115_LEVEL_CHANNEL);
    return adsDriver->getVoltage(rawValue);
}

void LevelDetector::calibrateEmpty() {
    float voltage = getFilteredVoltage();
    // Устанавливаем порог немного выше пустого состояния
    // (например, 50% от максимального напряжения)
    thresholdVoltage = voltage + 0.3f;  // Эмпирическое значение
    
    Serial.print("LevelDetector: Calibrated empty state: ");
    Serial.print(voltage, 3);
    Serial.print("V, threshold set to: ");
    Serial.print(thresholdVoltage, 3);
    Serial.println("V");
}

void LevelDetector::calibrateFull() {
    float voltage = getFilteredVoltage();
    // Устанавливаем порог немного ниже полного состояния
    thresholdVoltage = voltage * 0.9f;  // 90% от полного уровня
    
    Serial.print("LevelDetector: Calibrated full state: ");
    Serial.print(voltage, 3);
    Serial.print("V, threshold set to: ");
    Serial.print(thresholdVoltage, 3);
    Serial.println("V");
}
