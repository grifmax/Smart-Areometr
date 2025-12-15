#include "CapacitiveSensor.h"
#include "config.h"

CapacitiveSensor::CapacitiveSensor(uint8_t sensorPin, uint16_t numSamples, uint16_t sampleDelay)
    : pin(sensorPin), samples(numSamples), delayMs(sampleDelay) {
    waterValue = CALIBRATION_WATER_DEFAULT;
    alcoholValue = CALIBRATION_ALCOHOL_DEFAULT;
}

void CapacitiveSensor::begin() {
    // Настройка touch pin
    // ESP32-C3 автоматически настраивает пины при первом вызове touchRead()
    Serial.println("Capacitive sensor initialized on pin " + String(pin));

    // Выполняем первое считывание для инициализации
    touchRead(pin);
    delay(100);
}

uint16_t CapacitiveSensor::getRawValue() {
    uint32_t sum = 0;

    for (uint16_t i = 0; i < samples; i++) {
        sum += touchRead(pin);
        if (delayMs > 0) {
            delay(delayMs);
        }
    }

    return sum / samples;
}

void CapacitiveSensor::calibrateWater() {
    Serial.println("Calibrating for water (0% alcohol)...");
    Serial.println("Please ensure sensor is in pure water");
    delay(3000);  // Дать время на подготовку

    waterValue = getRawValue();
    Serial.println("Water calibration value: " + String(waterValue));
}

void CapacitiveSensor::calibrateAlcohol() {
    Serial.println("Calibrating for pure alcohol (100%)...");
    Serial.println("Please ensure sensor is in pure alcohol");
    delay(3000);  // Дать время на подготовку

    alcoholValue = getRawValue();
    Serial.println("Alcohol calibration value: " + String(alcoholValue));
}

float CapacitiveSensor::measureAlcoholPercent() {
    if (!isCalibrated()) {
        Serial.println("ERROR: Sensor not calibrated!");
        return -1.0f;
    }

    uint16_t rawValue = getRawValue();

    // Линейная интерполяция между водой и спиртом
    // Примечание: на практике зависимость может быть нелинейной,
    // возможно потребуется калибровочная кривая
    float percent = 0.0f;

    if (waterValue != alcoholValue) {
        percent = ((float)(rawValue - waterValue) / (alcoholValue - waterValue)) * 100.0f;

        // Ограничиваем диапазон 0-100%
        if (percent < 0.0f) percent = 0.0f;
        if (percent > 100.0f) percent = 100.0f;
    }

    Serial.println("Raw: " + String(rawValue) + " | Alcohol: " + String(percent) + "%");

    return percent;
}

uint16_t CapacitiveSensor::readRaw() {
    return getRawValue();
}

void CapacitiveSensor::setCalibration(float water, float alcohol) {
    waterValue = water;
    alcoholValue = alcohol;
    Serial.println("Calibration set - Water: " + String(water) + ", Alcohol: " + String(alcohol));
}

void CapacitiveSensor::getCalibration(float &water, float &alcohol) {
    water = waterValue;
    alcohol = alcoholValue;
}

bool CapacitiveSensor::isCalibrated() {
    // Проверяем, что значения отличаются от дефолтных и друг от друга
    return (waterValue != CALIBRATION_WATER_DEFAULT ||
            alcoholValue != CALIBRATION_ALCOHOL_DEFAULT) &&
           (waterValue != alcoholValue);
}
