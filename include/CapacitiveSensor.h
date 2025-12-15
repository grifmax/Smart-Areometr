#ifndef CAPACITIVE_SENSOR_H
#define CAPACITIVE_SENSOR_H

#include <Arduino.h>

/**
 * @brief Класс для работы с емкостным датчиком
 *
 * Использует встроенную функцию touchRead() ESP32-C3 для измерения
 * емкости жидкости. Диэлектрическая проницаемость зависит от
 * содержания алкоголя в растворе.
 */
class CapacitiveSensor {
private:
    uint8_t pin;
    float waterValue;      // Калибровочное значение для воды (0% алкоголя)
    float alcoholValue;    // Калибровочное значение для чистого спирта (100%)

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
    CapacitiveSensor(uint8_t sensorPin, uint16_t numSamples = 100, uint16_t sampleDelay = 10);

    /**
     * @brief Инициализация датчика
     */
    void begin();

    /**
     * @brief Калибровка на воде (0% алкоголя)
     */
    void calibrateWater();

    /**
     * @brief Калибровка на чистом спирте (100% алкоголя)
     */
    void calibrateAlcohol();

    /**
     * @brief Измерить процент алкоголя
     * @return Процент алкоголя (0-100)
     */
    float measureAlcoholPercent();

    /**
     * @brief Получить сырое значение емкости
     * @return Значение емкости
     */
    uint16_t readRaw();

    /**
     * @brief Установить калибровочные значения
     */
    void setCalibration(float water, float alcohol);

    /**
     * @brief Получить калибровочные значения
     */
    void getCalibration(float &water, float &alcohol);

    /**
     * @brief Проверка, откалиброван ли датчик
     */
    bool isCalibrated();
};

#endif // CAPACITIVE_SENSOR_H
