#ifndef MULTI_SENSOR_MANAGER_H
#define MULTI_SENSOR_MANAGER_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include "config.h"
#include "CapacitiveSensorV2.h"
#include "TemperatureCompensation.h"

#ifdef USE_ADS1115
#include "ADS1115Driver.h"
#endif

/**
 * @brief Структура данных одного датчика
 */
struct SensorData {
    uint8_t id;
    String name;
    float alcohol;          // Крепость (%)
    float temperature;      // Температура (°C)
    uint8_t stability;      // Стабильность (0-100%)
    uint16_t rawValue;      // Сырое значение
    bool isActive;          // Активен ли датчик
    bool isCalibrated;      // Откалиброван ли датчик
    unsigned long lastUpdate; // Время последнего обновления
    
    SensorData() : id(0), name(""), alcohol(0.0f), temperature(20.0f),
                   stability(0), rawValue(0), isActive(false),
                   isCalibrated(false), lastUpdate(0) {}
};

/**
 * @brief Менеджер для управления несколькими датчиками
 * 
 * Поддерживает до 4 датчиков через ADS1115 (4 канала)
 * Каждый датчик имеет свою калибровку и независимые измерения
 */
class MultiSensorManager {
private:
    static const uint8_t MAX_SENSORS = 4;
    SensorData sensors[MAX_SENSORS];
    uint8_t activeSensorCount;
    
#ifdef USE_ADS1115
    ADS1115Driver* ads1115Driver;
#endif
    
    // Массивы для хранения датчиков (если нужно несколько CapacitiveSensorV2)
    // Пока используем один датчик с переключением каналов
    
    /**
     * @brief Получить каналы ADS1115 для датчика
     */
    void getSensorChannels(uint8_t sensorId, uint8_t& ch1, uint8_t& ch2);
    
    /**
     * @brief Измерить датчик по ID
     */
    bool measureSensor(uint8_t sensorId);

public:
    /**
     * @brief Конструктор
     */
    MultiSensorManager();
    
    /**
     * @brief Инициализация менеджера
     * @param adsDriver Указатель на драйвер ADS1115 (если используется)
     * @return true если успешно
     */
    bool begin(ADS1115Driver* adsDriver = nullptr);
    
    /**
     * @brief Настроить датчик
     * @param sensorId ID датчика (0-3)
     * @param name Название датчика
     * @param channel1 Первый канал ADS1115 (0-3)
     * @param channel2 Второй канал ADS1115 (0-3)
     * @param enabled Включен ли датчик
     */
    void configureSensor(uint8_t sensorId, const String& name, 
                        uint8_t channel1, uint8_t channel2, bool enabled = true);
    
    /**
     * @brief Обновить измерения всех активных датчиков
     */
    void update();
    
    /**
     * @brief Обновить конкретный датчик
     * @param sensorId ID датчика (0-3)
     * @return true если успешно
     */
    bool updateSensor(uint8_t sensorId);
    
    /**
     * @brief Получить данные датчика
     * @param sensorId ID датчика (0-3)
     * @return Данные датчика или пустые данные при ошибке
     */
    const SensorData& getSensorData(uint8_t sensorId) const;
    
    /**
     * @brief Получить количество активных датчиков
     */
    uint8_t getActiveSensorCount() const { return activeSensorCount; }
    
    /**
     * @brief Получить JSON со всеми данными датчиков
     */
    String getSensorsJSON() const;
    
    /**
     * @brief Включить/выключить датчик
     */
    void setSensorEnabled(uint8_t sensorId, bool enabled);
    
    /**
     * @brief Проверить, включен ли датчик
     */
    bool isSensorEnabled(uint8_t sensorId) const;
    
    /**
     * @brief Получить среднюю крепость по всем активным датчикам
     */
    float getAverageAlcohol() const;
    
    /**
     * @brief Получить среднюю температуру по всем активным датчикам
     */
    float getAverageTemperature() const;
    
    /**
     * @brief Обнаружить аномалии (расхождение показаний)
     * @param threshold Порог расхождения (%)
     * @return true если обнаружены аномалии
     */
    bool detectAnomalies(float threshold = 5.0f) const;
};

#endif // MULTI_SENSOR_MANAGER_H

