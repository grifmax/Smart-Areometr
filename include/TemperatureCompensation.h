#ifndef TEMPERATURE_COMPENSATION_H
#define TEMPERATURE_COMPENSATION_H

#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "config.h"

/**
 * @brief Класс для работы с температурным датчиком и компенсации результатов
 *
 * Использует датчик DS18B20 для измерения температуры раствора
 * и применяет температурную компенсацию к измерениям алкоголя
 */
class TemperatureCompensation {
private:
    OneWire *oneWire;
    DallasTemperature *sensors;
    uint8_t pin;

    float currentTemperature;
    float referenceTemperature;  // Эталонная температура для компенсации
    float compensationCoeff;     // Коэффициент компенсации (%/°C)

    bool sensorAvailable;

public:
    /**
     * @brief Конструктор
     * @param sensorPin Пин датчика температуры (OneWire)
     * @param refTemp Эталонная температура (°C)
     * @param coeff Коэффициент компенсации (%/°C)
     */
    TemperatureCompensation(uint8_t sensorPin,
                          float refTemp = TEMP_REFERENCE,
                          float coeff = TEMP_COEFFICIENT);

    /**
     * @brief Деструктор
     */
    ~TemperatureCompensation();

    /**
     * @brief Инициализация датчика
     * @return true если датчик найден и инициализирован
     */
    bool begin();

    /**
     * @brief Измерить текущую температуру
     * @return Температура в градусах Цельсия
     */
    float measureTemperature();

    /**
     * @brief Получить последнее измеренное значение температуры
     */
    float getCurrentTemperature() const;

    /**
     * @brief Применить температурную компенсацию к измерению алкоголя
     * @param alcoholPercent Измеренный процент алкоголя
     * @return Скомпенсированный процент алкоголя
     */
    float compensate(float alcoholPercent);

    /**
     * @brief Применить температурную компенсацию с конкретной температурой
     * @param alcoholPercent Измеренный процент алкоголя
     * @param temperature Температура измерения
     * @return Скомпенсированный процент алкоголя
     */
    float compensate(float alcoholPercent, float temperature);

    /**
     * @brief Проверить доступность датчика
     */
    bool isAvailable() const;

    /**
     * @brief Установить эталонную температуру
     */
    void setReferenceTemperature(float temp);

    /**
     * @brief Установить коэффициент компенсации
     */
    void setCompensationCoefficient(float coeff);

    /**
     * @brief Получить температуру в Фаренгейтах
     */
    float getTemperatureFahrenheit() const;
};

#endif // TEMPERATURE_COMPENSATION_H
