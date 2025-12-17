#include "CapacitiveSensorV2.h"
#include "config.h"
#include <Arduino.h>
#include <math.h>

CapacitiveSensorV2::CapacitiveSensorV2(uint8_t sensorPin, uint16_t numSamples, uint16_t sampleDelay)
    : pin(sensorPin), samples(numSamples), delayMs(sampleDelay) {
    memset(&lastStats, 0, sizeof(lastStats));
}

void CapacitiveSensorV2::begin() {
    // Настройка touch pin
    Serial.println("Capacitive sensor V2 initialized on pin " + String(pin));

#if !defined(CONFIG_IDF_TARGET_ESP32C3) && !defined(CONFIG_IDF_TARGET_ESP32C6)
    // Только для ESP32 с аппаратным модулем тачсенсора
    touchRead(pin);
    delay(100);
#else
    // Для ESP32-C3/C6 без тачсенсора - используем ADC
    pinMode(pin, INPUT);
    analogSetAttenuation(ADC_11db);  // Полный диапазон 0-3.3V
    delay(100);
#endif

    // Калибровка baseline для touch сенсора
    Serial.println("Calibrating touch sensor baseline...");
    uint32_t sum = 0;
    for (int i = 0; i < 50; i++) {
#if !defined(CONFIG_IDF_TARGET_ESP32C3) && !defined(CONFIG_IDF_TARGET_ESP32C6)
        sum += touchRead(pin);
#else
        sum += analogRead(pin);
#endif
        delay(10);
    }
    float baseline = sum / 50.0f;
    Serial.println("Touch sensor baseline: " + String(baseline));
}

uint16_t CapacitiveSensorV2::getRawValue() {
    // Массив для хранения выборок (для вычисления статистики)
    uint16_t* values = new uint16_t[samples];
    uint32_t sum = 0;
    uint16_t minVal = 65535;
    uint16_t maxVal = 0;

    for (uint16_t i = 0; i < samples; i++) {
#if !defined(CONFIG_IDF_TARGET_ESP32C3) && !defined(CONFIG_IDF_TARGET_ESP32C6)
        values[i] = touchRead(pin);
#else
        values[i] = analogRead(pin);
#endif
        sum += values[i];

        if (values[i] < minVal) minVal = values[i];
        if (values[i] > maxVal) maxVal = values[i];

        if (delayMs > 0) {
            delay(delayMs);
        }
    }

    uint16_t average = sum / samples;

    // Вычисляем статистику
    lastStats.rawValue = average;
    lastStats.rawMin = minVal;
    lastStats.rawMax = maxVal;
    lastStats.rawStdDev = calculateStdDev(values, samples);

    // Вычисляем стабильность сигнала (0-100%)
    // Чем меньше разброс, тем выше стабильность
    float range = maxVal - minVal;
    if (range < 1) range = 1;
    lastStats.stability = constrain(100 - (range / average * 100), 0, 100);

    delete[] values;

    return average;
}

float CapacitiveSensorV2::calculateStdDev(uint16_t* values, uint16_t count) {
    if (count < 2) return 0.0f;

    // Вычисляем среднее
    float mean = 0;
    for (uint16_t i = 0; i < count; i++) {
        mean += values[i];
    }
    mean /= count;

    // Вычисляем дисперсию
    float variance = 0;
    for (uint16_t i = 0; i < count; i++) {
        float diff = values[i] - mean;
        variance += diff * diff;
    }
    variance /= (count - 1);

    return sqrt(variance);
}

bool CapacitiveSensorV2::addCalibrationPoint(float knownAlcohol, float temperature) {
    Serial.println("Adding calibration point for " + String(knownAlcohol) + "% at " + String(temperature) + "°C");
    Serial.println("Please ensure sensor is immersed in the solution...");

    delay(2000);  // Дать время на подготовку

    // Измеряем сырое значение
    uint16_t rawValue = getRawValue();

    Serial.println("Raw value: " + String(rawValue));
    Serial.println("Stability: " + String(lastStats.stability) + "%");
    Serial.println("StdDev: " + String(lastStats.rawStdDev, 2));

    // Проверка стабильности
    if (lastStats.stability < 70) {
        Serial.println("WARNING: Low signal stability (" + String(lastStats.stability) + "%)");
        Serial.println("Try to: 1) Remove air bubbles, 2) Wait longer, 3) Clean sensor");
    }

    // Добавляем точку в калибровочную кривую
    return calibrationCurve.addPoint(knownAlcohol, rawValue, temperature);
}

void CapacitiveSensorV2::clearCalibration() {
    calibrationCurve.clear();
    Serial.println("Calibration cleared");
}

uint8_t CapacitiveSensorV2::getCalibrationPointCount() {
    return calibrationCurve.getPointCount();
}

float CapacitiveSensorV2::measureAlcoholPercent(float currentTemp, bool applyTempCorrection) {
    if (!isCalibrated()) {
        Serial.println("ERROR: Sensor not calibrated!");
        return -1.0f;
    }

    // Получаем сырое значение
    uint16_t rawValue = getRawValue();

    Serial.print("Raw: " + String(rawValue));
    Serial.print(" | Stability: " + String(lastStats.stability) + "%");

    // Преобразуем сырое значение в крепость используя калибровочную кривую
    float alcoholPercent = calibrationCurve.rawToAlcohol(rawValue);

    if (alcoholPercent < 0) {
        Serial.println(" | ERROR: Failed to convert raw value");
        return -1.0f;
    }

    Serial.print(" | Alcohol (uncorrected): " + String(alcoholPercent, 1) + "%");

    // Применяем температурную коррекцию по ГОСТ
    if (applyTempCorrection) {
        float corrected = TemperatureCorrectionTable::getCorrection(
            alcoholPercent,
            currentTemp,
            20.0f  // Эталонная температура
        );

        Serial.print(" | Temp: " + String(currentTemp, 1) + "°C");
        Serial.print(" | Corrected: " + String(corrected, 1) + "%");

        alcoholPercent = corrected;
    }

    // Ограничиваем диапазон
    if (alcoholPercent < 0.0f) alcoholPercent = 0.0f;
    if (alcoholPercent > 100.0f) alcoholPercent = 100.0f;

    Serial.println(" | Final: " + String(alcoholPercent, 1) + "%");

    return alcoholPercent;
}

uint16_t CapacitiveSensorV2::readRaw() {
    return getRawValue();
}

String CapacitiveSensorV2::exportCalibration() {
    return calibrationCurve.toJSON();
}

bool CapacitiveSensorV2::importCalibration(const String& json) {
    return calibrationCurve.fromJSON(json);
}

bool CapacitiveSensorV2::isCalibrated() {
    return calibrationCurve.isValid();
}

CapacitiveSensorV2::CalibrationInfo CapacitiveSensorV2::getCalibrationInfo() {
    CalibrationInfo info;
    info.pointCount = calibrationCurve.getPointCount();
    info.valid = calibrationCurve.isValid();

    if (info.pointCount == 0) {
        info.minAlcohol = 0;
        info.maxAlcohol = 0;
        info.minRaw = 0;
        info.maxRaw = 0;
        return info;
    }

    // Находим минимум и максимум
    info.minAlcohol = 1000;
    info.maxAlcohol = -1;
    info.minRaw = 65535;
    info.maxRaw = 0;

    for (uint8_t i = 0; i < info.pointCount; i++) {
        CalibrationPoint point = calibrationCurve.getPoint(i);

        if (point.alcoholPercent < info.minAlcohol) info.minAlcohol = point.alcoholPercent;
        if (point.alcoholPercent > info.maxAlcohol) info.maxAlcohol = point.alcoholPercent;
        if (point.rawValue < info.minRaw) info.minRaw = point.rawValue;
        if (point.rawValue > info.maxRaw) info.maxRaw = point.rawValue;
    }

    return info;
}

float CapacitiveSensorV2::predictRawValue(float alcoholPercent) {
    return calibrationCurve.alcoholToRaw(alcoholPercent);
}

CapacitiveSensorV2::MeasurementStats CapacitiveSensorV2::getLastMeasurementStats() {
    return lastStats;
}
