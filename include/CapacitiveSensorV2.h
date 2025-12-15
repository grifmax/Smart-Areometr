#ifndef CAPACITIVE_SENSOR_V2_H
#define CAPACITIVE_SENSOR_V2_H

#include <Arduino.h>
#include "CalibrationTables.h"

/**
 * @brief Улучшенный класс для работы с емкостным датчиком
 *
 * Версия 2.0 с многоточечной калибровкой и точной температурной коррекцией
 */
class CapacitiveSensorV2 {
private:
    uint8_t pin;
    CalibrationCurve calibrationCurve;

    uint16_t samples;      // Количество выборок для усреднения
    uint16_t delayMs;      // Задержка между выборками

    /**
     * @brief Получить сырое значение емкости (усредненное)
     */
    uint16_t getRawValue();

public:
    /**
     * @brief Конструктор
     * @param sensorPin Пин датчика (должен поддерживать touch)
     * @param numSamples Количество выборок для усреднения
     * @param sampleDelay Задержка между выборками (мс)
     */
    CapacitiveSensorV2(uint8_t sensorPin, uint16_t numSamples = 100, uint16_t sampleDelay = 10);

    /**
     * @brief Инициализация датчика
     */
    void begin();

    /**
     * @brief Добавить калибровочную точку
     * @param knownAlcohol Известная крепость раствора (%)
     * @param temperature Температура раствора (°C)
     * @return true если точка добавлена успешно
     */
    bool addCalibrationPoint(float knownAlcohol, float temperature = 20.0f);

    /**
     * @brief Очистить все калибровочные точки
     */
    void clearCalibration();

    /**
     * @brief Получить количество калибровочных точек
     */
    uint8_t getCalibrationPointCount();

    /**
     * @brief Измерить процент алкоголя
     * @param currentTemp Текущая температура раствора (°C)
     * @param applyTempCorrection Применять ли температурную коррекцию по ГОСТ
     * @return Процент алкоголя (0-100) или -1 при ошибке
     */
    float measureAlcoholPercent(float currentTemp = 20.0f, bool applyTempCorrection = true);

    /**
     * @brief Получить сырое значение емкости
     * @return Значение емкости
     */
    uint16_t readRaw();

    /**
     * @brief Сохранить калибровку в JSON
     */
    String exportCalibration();

    /**
     * @brief Загрузить калибровку из JSON
     */
    bool importCalibration(const String& json);

    /**
     * @brief Проверка, откалиброван ли датчик
     */
    bool isCalibrated();

    /**
     * @brief Получить информацию о калибровке
     */
    struct CalibrationInfo {
        uint8_t pointCount;
        float minAlcohol;
        float maxAlcohol;
        float minRaw;
        float maxRaw;
        bool valid;
    };

    CalibrationInfo getCalibrationInfo();

    /**
     * @brief Предсказать ожидаемое сырое значение для заданной крепости
     * Полезно для проверки калибровки
     */
    float predictRawValue(float alcoholPercent);

    /**
     * @brief Получить статистику измерений
     */
    struct MeasurementStats {
        uint16_t rawValue;
        float rawStdDev;      // Стандартное отклонение
        uint16_t rawMin;
        uint16_t rawMax;
        uint8_t stability;    // 0-100%, качество сигнала
    };

    MeasurementStats getLastMeasurementStats();

private:
    MeasurementStats lastStats;

    /**
     * @brief Вычислить стандартное отклонение массива
     */
    float calculateStdDev(uint16_t* values, uint16_t count);
};

#endif // CAPACITIVE_SENSOR_V2_H
