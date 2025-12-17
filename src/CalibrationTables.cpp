#include <Arduino.h>
#include <ArduinoJson.h>
#include <algorithm>
#include "CalibrationTables.h"

// ============================================================================
// Таблица температурной коррекции (ГОСТ 3639-79)
// ============================================================================

// Таблица поправок для перевода крепости при температуре измерения к 20°C
// Строки: температура от 0 до 40°C (шаг 2°C)
// Столбцы: крепость от 0 до 100% (шаг 10%)
const float TemperatureCorrectionTable::correctionTable[21][11] = {
    // T=0°C:   0%    10%   20%   30%   40%   50%   60%   70%   80%   90%  100%
    /*  0°C */ {0.00, 0.80, 1.50, 2.10, 2.60, 3.00, 3.30, 3.50, 3.60, 3.60, 3.50},
    /*  2°C */ {0.00, 0.64, 1.20, 1.68, 2.08, 2.40, 2.64, 2.80, 2.88, 2.88, 2.80},
    /*  4°C */ {0.00, 0.48, 0.90, 1.26, 1.56, 1.80, 1.98, 2.10, 2.16, 2.16, 2.10},
    /*  6°C */ {0.00, 0.32, 0.60, 0.84, 1.04, 1.20, 1.32, 1.40, 1.44, 1.44, 1.40},
    /*  8°C */ {0.00, 0.16, 0.30, 0.42, 0.52, 0.60, 0.66, 0.70, 0.72, 0.72, 0.70},
    /* 10°C */ {0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00},
    /* 12°C */ {0.00,-0.16,-0.30,-0.42,-0.52,-0.60,-0.66,-0.70,-0.72,-0.72,-0.70},
    /* 14°C */ {0.00,-0.32,-0.60,-0.84,-1.04,-1.20,-1.32,-1.40,-1.44,-1.44,-1.40},
    /* 16°C */ {0.00,-0.48,-0.90,-1.26,-1.56,-1.80,-1.98,-2.10,-2.16,-2.16,-2.10},
    /* 18°C */ {0.00,-0.64,-1.20,-1.68,-2.08,-2.40,-2.64,-2.80,-2.88,-2.88,-2.80},
    /* 20°C */ {0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00},  // Эталонная
    /* 22°C */ {0.00, 0.64, 1.20, 1.68, 2.08, 2.40, 2.64, 2.80, 2.88, 2.88, 2.80},
    /* 24°C */ {0.00, 0.80, 1.50, 2.10, 2.60, 3.00, 3.30, 3.50, 3.60, 3.60, 3.50},
    /* 26°C */ {0.00, 0.96, 1.80, 2.52, 3.12, 3.60, 3.96, 4.20, 4.32, 4.32, 4.20},
    /* 28°C */ {0.00, 1.12, 2.10, 2.94, 3.64, 4.20, 4.62, 4.90, 5.04, 5.04, 4.90},
    /* 30°C */ {0.00, 1.28, 2.40, 3.36, 4.16, 4.80, 5.28, 5.60, 5.76, 5.76, 5.60},
    /* 32°C */ {0.00, 1.44, 2.70, 3.78, 4.68, 5.40, 5.94, 6.30, 6.48, 6.48, 6.30},
    /* 34°C */ {0.00, 1.60, 3.00, 4.20, 5.20, 6.00, 6.60, 7.00, 7.20, 7.20, 7.00},
    /* 36°C */ {0.00, 1.76, 3.30, 4.62, 5.72, 6.60, 7.26, 7.70, 7.92, 7.92, 7.70},
    /* 38°C */ {0.00, 1.92, 3.60, 5.04, 6.24, 7.20, 7.92, 8.40, 8.64, 8.64, 8.40},
    /* 40°C */ {0.00, 2.08, 3.90, 5.46, 6.76, 7.80, 8.58, 9.10, 9.36, 9.36, 9.10}
};

float TemperatureCorrectionTable::getCorrection(float alcoholPercent, float currentTemp, float referenceTemp) {
    // Ограничиваем диапазон
    if (currentTemp < 0.0f) currentTemp = 0.0f;
    if (currentTemp > 40.0f) currentTemp = 40.0f;
    if (alcoholPercent < 0.0f) alcoholPercent = 0.0f;
    if (alcoholPercent > 100.0f) alcoholPercent = 100.0f;

    // Если температура близка к эталонной, коррекция не нужна
    if (abs(currentTemp - referenceTemp) < 0.5f) {
        return alcoholPercent;
    }

    // Получаем поправку из таблицы (билинейная интерполяция)
    float correction = bilinearInterpolation(currentTemp, alcoholPercent);

    // Если температура выше эталонной, показания занижены (прибавляем)
    // Если ниже - завышены (вычитаем)
    if (currentTemp > referenceTemp) {
        return alcoholPercent + correction;
    } else {
        return alcoholPercent - correction;
    }
}

float TemperatureCorrectionTable::bilinearInterpolation(float temp, float alcohol) {
    // Индексы для температуры (шаг 2°C, начало 0°C)
    float tempIndex = temp / 2.0f;
    int tempIdx0 = (int)tempIndex;
    int tempIdx1 = tempIdx0 + 1;

    if (tempIdx0 < 0) tempIdx0 = 0;
    if (tempIdx1 > 20) tempIdx1 = 20;
    if (tempIdx0 > 20) tempIdx0 = 20;

    float tempFrac = tempIndex - tempIdx0;

    // Индексы для крепости (шаг 10%, начало 0%)
    float alcoholIndex = alcohol / 10.0f;
    int alcIdx0 = (int)alcoholIndex;
    int alcIdx1 = alcIdx0 + 1;

    if (alcIdx0 < 0) alcIdx0 = 0;
    if (alcIdx1 > 10) alcIdx1 = 10;
    if (alcIdx0 > 10) alcIdx0 = 10;

    float alcFrac = alcoholIndex - alcIdx0;

    // Билинейная интерполяция
    float c00 = correctionTable[tempIdx0][alcIdx0];
    float c10 = correctionTable[tempIdx1][alcIdx0];
    float c01 = correctionTable[tempIdx0][alcIdx1];
    float c11 = correctionTable[tempIdx1][alcIdx1];

    float c0 = c00 * (1 - alcFrac) + c01 * alcFrac;
    float c1 = c10 * (1 - alcFrac) + c11 * alcFrac;

    return c0 * (1 - tempFrac) + c1 * tempFrac;
}

// ============================================================================
// Калибровочная кривая
// ============================================================================

CalibrationCurve::CalibrationCurve() : pointCount(0), sorted(false) {
    memset(points, 0, sizeof(points));
}

bool CalibrationCurve::addPoint(float alcoholPercent, float rawValue, float temperature) {
    if (pointCount >= MAX_CALIBRATION_POINTS) {
        Serial.println("ERROR: Max calibration points reached");
        return false;
    }

    // Проверка на дубликаты по крепости
    for (uint8_t i = 0; i < pointCount; i++) {
        if (abs(points[i].alcoholPercent - alcoholPercent) < 0.1f) {
            // Обновляем существующую точку
            points[i].rawValue = rawValue;
            points[i].temperature = temperature;
            Serial.println("Updated existing calibration point at " + String(alcoholPercent) + "%");
            sorted = false;
            return true;
        }
    }

    // Добавляем новую точку
    points[pointCount].alcoholPercent = alcoholPercent;
    points[pointCount].rawValue = rawValue;
    points[pointCount].temperature = temperature;
    pointCount++;
    sorted = false;

    Serial.println("Added calibration point: " + String(alcoholPercent) + "% = " + String(rawValue));
    return true;
}

void CalibrationCurve::clear() {
    pointCount = 0;
    sorted = false;
    memset(points, 0, sizeof(points));
    Serial.println("Calibration curve cleared");
}

CalibrationPoint CalibrationCurve::getPoint(uint8_t index) const {
    if (index < pointCount) {
        return points[index];
    }
    return CalibrationPoint{0, 0, 0};
}

void CalibrationCurve::sortPoints() {
    if (sorted || pointCount < 2) return;

    // Сортировка пузырьком (достаточно для малого количества точек)
    for (uint8_t i = 0; i < pointCount - 1; i++) {
        for (uint8_t j = 0; j < pointCount - i - 1; j++) {
            if (points[j].alcoholPercent > points[j + 1].alcoholPercent) {
                CalibrationPoint temp = points[j];
                points[j] = points[j + 1];
                points[j + 1] = temp;
            }
        }
    }
    sorted = true;
}

float CalibrationCurve::linearInterpolate(float x, float x0, float y0, float x1, float y1) {
    if (abs(x1 - x0) < 0.001f) return y0;
    return y0 + (x - x0) * (y1 - y0) / (x1 - x0);
}

float CalibrationCurve::rawToAlcohol(float rawValue) {
    if (!isValid()) {
        Serial.println("ERROR: Invalid calibration curve");
        return -1.0f;
    }

    sortPoints();

    // Если значение вне диапазона, экстраполируем от крайних точек
    if (rawValue <= points[0].rawValue) {
        if (pointCount == 1) return points[0].alcoholPercent;
        // Экстраполяция от первых двух точек
        return linearInterpolate(rawValue,
                                points[0].rawValue, points[0].alcoholPercent,
                                points[1].rawValue, points[1].alcoholPercent);
    }

    if (rawValue >= points[pointCount - 1].rawValue) {
        if (pointCount == 1) return points[0].alcoholPercent;
        // Экстраполяция от последних двух точек
        return linearInterpolate(rawValue,
                                points[pointCount - 2].rawValue, points[pointCount - 2].alcoholPercent,
                                points[pointCount - 1].rawValue, points[pointCount - 1].alcoholPercent);
    }

    // Интерполяция между точками
    for (uint8_t i = 0; i < pointCount - 1; i++) {
        if (rawValue >= points[i].rawValue && rawValue <= points[i + 1].rawValue) {
            return linearInterpolate(rawValue,
                                    points[i].rawValue, points[i].alcoholPercent,
                                    points[i + 1].rawValue, points[i + 1].alcoholPercent);
        }
    }

    return -1.0f;
}

float CalibrationCurve::alcoholToRaw(float alcoholPercent) {
    if (!isValid()) {
        return -1.0f;
    }

    sortPoints();

    // Аналогично rawToAlcohol, но наоборот
    if (alcoholPercent <= points[0].alcoholPercent) {
        if (pointCount == 1) return points[0].rawValue;
        return linearInterpolate(alcoholPercent,
                                points[0].alcoholPercent, points[0].rawValue,
                                points[1].alcoholPercent, points[1].rawValue);
    }

    if (alcoholPercent >= points[pointCount - 1].alcoholPercent) {
        if (pointCount == 1) return points[0].rawValue;
        return linearInterpolate(alcoholPercent,
                                points[pointCount - 2].alcoholPercent, points[pointCount - 2].rawValue,
                                points[pointCount - 1].alcoholPercent, points[pointCount - 1].rawValue);
    }

    for (uint8_t i = 0; i < pointCount - 1; i++) {
        if (alcoholPercent >= points[i].alcoholPercent &&
            alcoholPercent <= points[i + 1].alcoholPercent) {
            return linearInterpolate(alcoholPercent,
                                    points[i].alcoholPercent, points[i].rawValue,
                                    points[i + 1].alcoholPercent, points[i + 1].rawValue);
        }
    }

    return -1.0f;
}

String CalibrationCurve::toJSON() {
    JsonDocument doc;
    JsonArray pointsArray = doc["points"].to<JsonArray>();

    for (uint8_t i = 0; i < pointCount; i++) {
        JsonObject point = pointsArray.add<JsonObject>();
        point["alcohol"] = points[i].alcoholPercent;
        point["raw"] = points[i].rawValue;
        point["temp"] = points[i].temperature;
    }

    String output;
    serializeJson(doc, output);
    return output;
}

bool CalibrationCurve::fromJSON(const String& json) {
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, json);

    if (error) {
        Serial.println("ERROR: Failed to parse calibration JSON");
        return false;
    }

    clear();

    JsonArray pointsArray = doc["points"].as<JsonArray>();
    for (JsonObject point : pointsArray) {
        float alcohol = point["alcohol"];
        float raw = point["raw"];
        float temp = point["temp"] | 20.0f;
        addPoint(alcohol, raw, temp);
    }

    Serial.println("Loaded " + String(pointCount) + " calibration points");
    return true;
}
