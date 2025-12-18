#include <Arduino.h>
#include "SerialCompat.h"
#include "BatteryMonitor.h"
#include "config.h"
#include <ArduinoJson.h>

BatteryMonitor::BatteryMonitor(uint8_t pin, float vMin, float vMax, uint8_t lowThr)
    : adcPin(pin), voltageMin(vMin), voltageMax(vMax), lowThreshold(lowThr),
      criticalThreshold(10), currentVoltage(0.0f), currentPercent(100),
      isCharging(false), lowBatteryWarning(false), lastUpdateTime(0), updateInterval(30000) {
}

bool BatteryMonitor::begin() {
    pinMode(adcPin, INPUT);
    
    // Первое измерение
    update();
    
    Serial.println("BatteryMonitor initialized");
    Serial.printf("  Pin: GPIO%d\n", adcPin);
    Serial.printf("  Voltage range: %.2f - %.2f V\n", voltageMin, voltageMax);
    Serial.printf("  Low threshold: %d%%\n", lowThreshold);
    
    return true;
}

void BatteryMonitor::update() {
    unsigned long now = millis();
    
    // Проверяем интервал обновления
    if (now - lastUpdateTime < updateInterval) {
        return;
    }
    
    lastUpdateTime = now;
    
    // Читаем напряжение
    currentVoltage = readVoltage();
    
    // Вычисляем процент заряда
    currentPercent = calculatePercent(currentVoltage);
    
    // Обновляем статус зарядки
    updateChargingStatus();
    
    // Проверяем предупреждения
    if (currentPercent <= lowThreshold && !lowBatteryWarning) {
        lowBatteryWarning = true;
        Serial.printf("WARNING: Low battery! %d%% (%.2fV)\n", currentPercent, currentVoltage);
    } else if (currentPercent > lowThreshold) {
        lowBatteryWarning = false;
    }
}

float BatteryMonitor::readVoltage() {
    // Читаем значение ADC (0-4095 для 12-bit на ESP32-C3)
    int adcValue = analogRead(adcPin);
    
    // ESP32-C3 ADC: 12-bit (0-4095), опорное напряжение обычно 3.3V
    // Но для точности нужно учитывать делитель напряжения, если он используется
    // Предполагаем, что напряжение батареи делится делителем (например, 2:1)
    
    // Для Li-Ion 3.7V батареи с максимальным напряжением 4.2V,
    // делитель обычно делает максимальное напряжение на ADC ~2.1V (4.2V / 2)
    // Но это зависит от конкретной схемы подключения
    
    // Базовое преобразование: ADC значение -> напряжение
    // Для ESP32-C3 с опорным напряжением 3.3V
    float voltageRaw = (adcValue / 4095.0f) * 3.3f;
    
    // Если используется делитель напряжения (например, R1=R2=10kΩ),
    // то напряжение батареи = voltageRaw * 2
    // Настраиваемый коэффициент делителя (по умолчанию 2.0 для стандартного делителя)
    float dividerRatio = 2.0f;  // Можно настроить в config.h
    
    float batteryVoltage = voltageRaw * dividerRatio;
    
    // Ограничиваем диапазон
    if (batteryVoltage < voltageMin) batteryVoltage = voltageMin;
    if (batteryVoltage > voltageMax) batteryVoltage = voltageMax;
    
    return batteryVoltage;
}

uint8_t BatteryMonitor::calculatePercent(float voltage) {
    // Линейная интерполяция между min и max напряжением
    // Для Li-Ion батареи кривая разряда не линейна, но для простоты используем линейную аппроксимацию
    
    if (voltage <= voltageMin) {
        return 0;
    }
    if (voltage >= voltageMax) {
        return 100;
    }
    
    float percent = ((voltage - voltageMin) / (voltageMax - voltageMin)) * 100.0f;
    
    // Более точная кривая для Li-Ion (опционально):
    // Используем приблизительную кривую заряда Li-Ion батареи
    // Но для начала используем простую линейную интерполяцию
    
    return (uint8_t)constrain(percent, 0, 100);
}

void BatteryMonitor::updateChargingStatus() {
    // Определение статуса зарядки зависит от схемы подключения
    // Если доступен отдельный пин для определения зарядки (например, от TP4056),
    // используем его. Иначе определяем по изменению напряжения.
    
    // Простая эвристика: если напряжение близко к максимальному и не падает,
    // возможно идет зарядка
    // Но это ненадежно, лучше использовать отдельный пин если доступен
    
    static float previousVoltage = 0.0f;
    
    if (currentVoltage >= voltageMax * 0.98f) {
        // Напряжение близко к максимальному - вероятно зарядка
        isCharging = true;
    } else if (currentVoltage < previousVoltage - 0.05f) {
        // Напряжение падает - разрядка
        isCharging = false;
    }
    // Иначе оставляем предыдущее значение
    
    previousVoltage = currentVoltage;
}

String BatteryMonitor::getStatusJSON() const {
    JsonDocument doc;
    
    doc["voltage"] = currentVoltage;
    doc["percent"] = currentPercent;
    doc["charging"] = isCharging;
    doc["low_battery"] = isLowBattery();
    doc["critical"] = isCriticalBattery();
    doc["voltage_min"] = voltageMin;
    doc["voltage_max"] = voltageMax;
    
    String json;
    serializeJson(doc, json);
    return json;
}

void BatteryMonitor::setThresholds(uint8_t low, uint8_t critical) {
    lowThreshold = low;
    criticalThreshold = critical;
    
    Serial.printf("Battery thresholds updated: low=%d%%, critical=%d%%\n", low, critical);
}
