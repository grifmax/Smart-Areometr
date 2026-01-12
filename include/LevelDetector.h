#ifndef LEVEL_DETECTOR_H
#define LEVEL_DETECTOR_H

#include <Arduino.h>
#include <functional>
#include "config.h"

// Forward declaration
class ADS1115Driver;

/**
 * @brief Детектор уровня жидкости в приемнике
 * 
 * Использует проводящий метод детекции через ADS1115 канал AIN2.
 * Определяет состояние переполнения на основе порогового напряжения.
 */
class LevelDetector {
private:
    ADS1115Driver* adsDriver;
    bool enabled;
    bool isOverflowing;
    bool lastOverflowState;
    
    float thresholdVoltage;  // Пороговое напряжение для срабатывания
    uint16_t debounceMs;     // Задержка перед срабатыванием
    unsigned long lastCheckTime;
    unsigned long overflowStartTime;
    
    // Фильтр для стабилизации показаний
    float filterBuffer[LEVEL_FILTER_SIZE];
    uint8_t filterIndex;
    uint8_t filterCount;
    
    // Callback при обнаружении переполнения
    std::function<void()> overflowCallback;
    std::function<void()> clearCallback;
    
    /**
     * @brief Получить отфильтрованное значение напряжения
     */
    float getFilteredVoltage();
    
    /**
     * @brief Обновить состояние переполнения с учетом дебаунсинга
     */
    void updateOverflowState(bool newState);

public:
    /**
     * @brief Конструктор
     */
    LevelDetector();
    
    /**
     * @brief Инициализация детектора
     * @param driver Указатель на драйвер ADS1115
     * @return true если инициализация успешна
     */
    bool begin(ADS1115Driver* driver);
    
    /**
     * @brief Обновить состояние детектора (вызывать периодически)
     */
    void update();
    
    /**
     * @brief Получить текущее состояние переполнения
     * @return true если обнаружено переполнение
     */
    bool isOverflow() const { return isOverflowing; }
    
    /**
     * @brief Получить текущее напряжение с канала AIN2
     * @return Напряжение в вольтах
     */
    float getCurrentVoltage();
    
    /**
     * @brief Установить пороговое напряжение
     * @param voltage Пороговое напряжение в вольтах
     */
    void setThreshold(float voltage);
    
    /**
     * @brief Получить текущий порог
     * @return Пороговое напряжение в вольтах
     */
    float getThreshold() const { return thresholdVoltage; }
    
    /**
     * @brief Включить/выключить детектор
     */
    void setEnabled(bool enable) { enabled = enable; }
    
    /**
     * @brief Проверить, включен ли детектор
     */
    bool isEnabled() const { return enabled; }
    
    /**
     * @brief Установить callback при обнаружении переполнения
     */
    void setOnOverflowCallback(std::function<void()> callback) {
        overflowCallback = callback;
    }
    
    /**
     * @brief Установить callback при очистке переполнения
     */
    void setOnClearCallback(std::function<void()> callback) {
        clearCallback = callback;
    }
    
    /**
     * @brief Калибровка порога: пустой приемник
     * Сохраняет текущее напряжение как базовое значение
     */
    void calibrateEmpty();
    
    /**
     * @brief Калибровка порога: полный приемник
     * Вычисляет порог на основе текущего напряжения
     */
    void calibrateFull();
    
    /**
     * @brief Получить отфильтрованное значение (для отладки)
     */
    float getRawVoltage() const;
};

#endif // LEVEL_DETECTOR_H
