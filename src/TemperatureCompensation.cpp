#include <Arduino.h>
#include "TemperatureCompensation.h"
#include "config.h"

TemperatureCompensation::TemperatureCompensation(uint8_t sensorPin, float refTemp, float coeff)
    : pin(sensorPin), referenceTemperature(refTemp), compensationCoeff(coeff),
      currentTemperature(refTemp), sensorAvailable(false) {
    oneWire = new OneWire(pin);
    sensors = new DallasTemperature(oneWire);
}

TemperatureCompensation::~TemperatureCompensation() {
    delete sensors;
    delete oneWire;
}

bool TemperatureCompensation::begin() {
    sensors->begin();

    // Проверяем наличие датчиков
    uint8_t deviceCount = sensors->getDeviceCount();

    if (deviceCount > 0) {
        sensorAvailable = true;
        Serial.println("Temperature sensor found. Devices: " + String(deviceCount));

        // Устанавливаем разрешение 12 бит для точности
        sensors->setResolution(12);

        // Первое измерение
        measureTemperature();

        return true;
    } else {
        sensorAvailable = false;
        Serial.println("WARNING: No temperature sensor found!");
        return false;
    }
}

float TemperatureCompensation::measureTemperature() {
    if (!sensorAvailable) {
        Serial.println("Temperature sensor not available");
        return currentTemperature;  // Возвращаем последнее значение
    }

    sensors->requestTemperatures();
    currentTemperature = sensors->getTempCByIndex(0);

    // Проверка на ошибку чтения
    if (currentTemperature == DEVICE_DISCONNECTED_C) {
        Serial.println("ERROR: Failed to read temperature");
        currentTemperature = referenceTemperature;  // Используем эталонную
    } else {
        Serial.println("Temperature: " + String(currentTemperature) + "°C");
    }

    return currentTemperature;
}

float TemperatureCompensation::getCurrentTemperature() const {
    return currentTemperature;
}

float TemperatureCompensation::compensate(float alcoholPercent) {
    return compensate(alcoholPercent, currentTemperature);
}

float TemperatureCompensation::compensate(float alcoholPercent, float temperature) {
    // Температурная компенсация:
    // Диэлектрическая проницаемость меняется с температурой
    // Применяем линейную коррекцию относительно эталонной температуры

    float tempDiff = temperature - referenceTemperature;
    float compensation = tempDiff * compensationCoeff;

    float compensatedPercent = alcoholPercent - compensation;

    // Ограничиваем диапазон
    if (compensatedPercent < 0.0f) compensatedPercent = 0.0f;
    if (compensatedPercent > 100.0f) compensatedPercent = 100.0f;

    Serial.println("Compensation: " + String(alcoholPercent) + "% -> " +
                   String(compensatedPercent) + "% (ΔT=" + String(tempDiff) + "°C)");

    return compensatedPercent;
}

bool TemperatureCompensation::isAvailable() const {
    return sensorAvailable;
}

void TemperatureCompensation::setReferenceTemperature(float temp) {
    referenceTemperature = temp;
    Serial.println("Reference temperature set to: " + String(temp) + "°C");
}

void TemperatureCompensation::setCompensationCoefficient(float coeff) {
    compensationCoeff = coeff;
    Serial.println("Compensation coefficient set to: " + String(coeff) + "%/°C");
}

float TemperatureCompensation::getTemperatureFahrenheit() const {
    return currentTemperature * 9.0f / 5.0f + 32.0f;
}
