#include <Arduino.h>
#include "SerialCompat.h"
#include "MultiSensorManager.h"
#include "config.h"

MultiSensorManager::MultiSensorManager()
    : activeSensorCount(0) {
#ifdef USE_ADS1115
    ads1115Driver = nullptr;
#endif
    
    // Инициализация датчиков
    for (uint8_t i = 0; i < MAX_SENSORS; i++) {
        sensors[i].id = i;
        sensors[i].name = "Датчик " + String(i + 1);
        sensors[i].isActive = false;
        sensors[i].isCalibrated = false;
    }
}

bool MultiSensorManager::begin(ADS1115Driver* adsDriver) {
#ifdef USE_ADS1115
    ads1115Driver = adsDriver;
    
    if (!adsDriver || !adsDriver->isInitialized()) {
        Serial.println("MultiSensorManager: ADS1115 не инициализирован");
        return false;
    }
    
    Serial.println("MultiSensorManager: Инициализирован с ADS1115");
    
    // Настройка датчиков по умолчанию
    // Датчик 0: каналы 0-1 (основной)
    configureSensor(0, "Основной датчик", 0, 1, true);
    
    // Датчик 1: каналы 2-3 (если используется)
    configureSensor(1, "Датчик 2", 2, 3, false);
    
    // Остальные датчики можно настроить через API
    return true;
#else
    Serial.println("MultiSensorManager: USE_ADS1115 отключен, мультисенсорный режим недоступен");
    return false;
#endif
}

void MultiSensorManager::configureSensor(uint8_t sensorId, const String& name,
                                         uint8_t channel1, uint8_t channel2, bool enabled) {
    if (sensorId >= MAX_SENSORS) {
        return;
    }
    
    sensors[sensorId].name = name;
    sensors[sensorId].isActive = enabled;
    
    // Сохраняем каналы в структуре (можно использовать для будущей реализации)
    // Пока используем один CapacitiveSensorV2 с переключением
    
    if (enabled && !sensors[sensorId].isActive) {
        activeSensorCount++;
    } else if (!enabled && sensors[sensorId].isActive) {
        activeSensorCount--;
    }
    
    Serial.printf("MultiSensorManager: Датчик %d настроен: %s (каналы %d-%d, %s)\n",
                  sensorId, name.c_str(), channel1, channel2, enabled ? "включен" : "выключен");
}

void MultiSensorManager::update() {
    // Обновляем все активные датчики
    for (uint8_t i = 0; i < MAX_SENSORS; i++) {
        if (sensors[i].isActive) {
            updateSensor(i);
        }
    }
}

bool MultiSensorManager::updateSensor(uint8_t sensorId) {
    if (sensorId >= MAX_SENSORS || !sensors[sensorId].isActive) {
        return false;
    }
    
#ifdef USE_ADS1115
    if (!ads1115Driver || !ads1115Driver->isInitialized()) {
        return false;
    }
    
    // Для текущей реализации используем основной датчик
    // В будущем можно добавить переключение каналов для каждого датчика
    // Пока все датчики используют те же каналы, что и основной
    
    // Обновляем время последнего обновления
    sensors[sensorId].lastUpdate = millis();
    
    // Данные будут обновляться через основной CapacitiveSensorV2
    // Это упрощенная реализация - в полной версии нужны отдельные экземпляры
    // или переключение каналов
    
    return true;
#else
    return false;
#endif
}

const SensorData& MultiSensorManager::getSensorData(uint8_t sensorId) const {
    static SensorData emptyData;
    
    if (sensorId >= MAX_SENSORS) {
        return emptyData;
    }
    
    return sensors[sensorId];
}

String MultiSensorManager::getSensorsJSON() const {
    JsonDocument doc;
    doc["sensor_count"] = activeSensorCount;
    doc["max_sensors"] = MAX_SENSORS;
    
    JsonArray sensorsArray = doc["sensors"].to<JsonArray>();
    
    for (uint8_t i = 0; i < MAX_SENSORS; i++) {
        JsonObject sensorObj = sensorsArray.add<JsonObject>();
        sensorObj["id"] = sensors[i].id;
        sensorObj["name"] = sensors[i].name;
        sensorObj["alcohol"] = sensors[i].alcohol;
        sensorObj["temperature"] = sensors[i].temperature;
        sensorObj["stability"] = sensors[i].stability;
        sensorObj["raw_value"] = sensors[i].rawValue;
        sensorObj["active"] = sensors[i].isActive;
        sensorObj["calibrated"] = sensors[i].isCalibrated;
        sensorObj["last_update"] = sensors[i].lastUpdate;
    }
    
    // Средние значения
    doc["average_alcohol"] = getAverageAlcohol();
    doc["average_temperature"] = getAverageTemperature();
    doc["anomalies_detected"] = detectAnomalies();
    
    String json;
    serializeJson(doc, json);
    return json;
}

void MultiSensorManager::setSensorEnabled(uint8_t sensorId, bool enabled) {
    if (sensorId >= MAX_SENSORS) {
        return;
    }
    
    bool wasActive = sensors[sensorId].isActive;
    sensors[sensorId].isActive = enabled;
    
    if (enabled && !wasActive) {
        activeSensorCount++;
    } else if (!enabled && wasActive) {
        activeSensorCount--;
    }
}

bool MultiSensorManager::isSensorEnabled(uint8_t sensorId) const {
    if (sensorId >= MAX_SENSORS) {
        return false;
    }
    
    return sensors[sensorId].isActive;
}

float MultiSensorManager::getAverageAlcohol() const {
    float sum = 0.0f;
    uint8_t count = 0;
    
    for (uint8_t i = 0; i < MAX_SENSORS; i++) {
        if (sensors[i].isActive && sensors[i].isCalibrated) {
            sum += sensors[i].alcohol;
            count++;
        }
    }
    
    return (count > 0) ? (sum / count) : 0.0f;
}

float MultiSensorManager::getAverageTemperature() const {
    float sum = 0.0f;
    uint8_t count = 0;
    
    for (uint8_t i = 0; i < MAX_SENSORS; i++) {
        if (sensors[i].isActive) {
            sum += sensors[i].temperature;
            count++;
        }
    }
    
    return (count > 0) ? (sum / count) : 20.0f;
}

bool MultiSensorManager::detectAnomalies(float threshold) const {
    if (activeSensorCount < 2) {
        return false;  // Нужно минимум 2 датчика для сравнения
    }
    
    // Находим среднее значение крепости
    float avg = getAverageAlcohol();
    
    // Проверяем отклонение каждого датчика от среднего
    for (uint8_t i = 0; i < MAX_SENSORS; i++) {
        if (sensors[i].isActive && sensors[i].isCalibrated) {
            float deviation = abs(sensors[i].alcohol - avg);
            if (deviation > threshold) {
                return true;  // Обнаружено значительное расхождение
            }
        }
    }
    
    return false;
}

void MultiSensorManager::getSensorChannels(uint8_t sensorId, uint8_t& ch1, uint8_t& ch2) {
    // Маппинг датчиков на каналы ADS1115
    switch (sensorId) {
        case 0:
            ch1 = 0;
            ch2 = 1;
            break;
        case 1:
            ch1 = 2;
            ch2 = 3;
            break;
        case 2:
            // Можно использовать комбинации каналов
            ch1 = 0;
            ch2 = 2;
            break;
        case 3:
            ch1 = 1;
            ch2 = 3;
            break;
        default:
            ch1 = 0;
            ch2 = 1;
            break;
    }
}

