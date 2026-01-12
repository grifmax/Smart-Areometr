#ifndef BATTERY_MONITOR_H
#define BATTERY_MONITOR_H

#include <Arduino.h>

/**
 * @brief Класс для мониторинга заряда батареи
 * 
 * Измеряет напряжение батареи через ADC, вычисляет процент заряда,
 * отслеживает статус зарядки и предоставляет предупреждения о низком заряде.
 */
class BatteryMonitor {
private:
    uint8_t adcPin;
    float voltageMin;          // Минимальное напряжение (В) - полный разряд
    float voltageMax;          // Максимальное напряжение (В) - полный заряд
    uint8_t lowThreshold;      // Порог низкого заряда (%)
    uint8_t criticalThreshold; // Порог критического заряда (%)
    
    float currentVoltage;      // Текущее напряжение (В)
    uint8_t currentPercent;    // Текущий процент заряда (0-100)
    bool isCharging;           // Статус зарядки
    bool lowBatteryWarning;    // Флаг предупреждения о низком заряде
    
    unsigned long lastUpdateTime;
    uint16_t updateInterval;   // Интервал обновления (мс)
    
    /**
     * @brief Прочитать напряжение с ADC
     * @return Напряжение в вольтах
     */
    float readVoltage();
    
    /**
     * @brief Вычислить процент заряда на основе напряжения
     * @param voltage Напряжение батареи (В)
     * @return Процент заряда (0-100)
     */
    uint8_t calculatePercent(float voltage);
    
    /**
     * @brief Проверить статус зарядки (если доступно)
     */
    void updateChargingStatus();

public:
    /**
     * @brief Конструктор
     * @param pin ADC пин для измерения напряжения
     * @param vMin Минимальное напряжение (В)
     * @param vMax Максимальное напряжение (В)
     * @param lowThr Порог низкого заряда (%)
     */
    BatteryMonitor(uint8_t pin, float vMin = 3.0f, float vMax = 4.2f, uint8_t lowThr = 20);
    
    /**
     * @brief Инициализация монитора
     * @return true если успешно
     */
    bool begin();
    
    /**
     * @brief Обновить показания (вызывать периодически)
     */
    void update();
    
    /**
     * @brief Получить текущее напряжение батареи
     * @return Напряжение в вольтах
     */
    float getVoltage() const { return currentVoltage; }
    
    /**
     * @brief Получить текущий процент заряда
     * @return Процент заряда (0-100)
     */
    uint8_t getPercent() const { return currentPercent; }
    
    /**
     * @brief Проверить, заряжается ли батарея
     * @return true если батарея заряжается
     */
    bool isBatteryCharging() const { return isCharging; }
    
    /**
     * @brief Проверить, низкий ли заряд батареи
     * @return true если заряд ниже порога
     */
    bool isLowBattery() const { return currentPercent <= lowThreshold; }
    
    /**
     * @brief Проверить, критический ли заряд батареи
     * @return true если заряд критически низкий
     */
    bool isCriticalBattery() const { return currentPercent <= criticalThreshold; }
    
    /**
     * @brief Получить статус в виде JSON строки
     * @return JSON с данными о батарее
     */
    String getStatusJSON() const;
    
    /**
     * @brief Установить пороги предупреждений
     * @param low Порог низкого заряда (%)
     * @param critical Порог критического заряда (%)
     */
    void setThresholds(uint8_t low, uint8_t critical);
};

#endif // BATTERY_MONITOR_H
