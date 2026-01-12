#ifndef POWER_MANAGER_H
#define POWER_MANAGER_H

#include <Arduino.h>
#include <esp_sleep.h>
#include <Preferences.h>
#include "config.h"

/**
 * @brief Менеджер питания для управления Deep Sleep режимом
 * 
 * Обеспечивает экономию энергии через Deep Sleep режим с сохранением
 * состояния в RTC памяти и пробуждением по таймеру или внешнему прерыванию.
 */
class PowerManager {
private:
    bool deepSleepEnabled;
    unsigned long sleepIntervalMs;  // Интервал сна в миллисекундах
    unsigned long lastWakeTime;
    Preferences preferences;
    
    // Состояние для сохранения в RTC памяти
    struct SleepState {
        uint32_t wakeCount;
        uint32_t totalSleepTime;
        float lastAlcohol;
        float lastTemperature;
        uint8_t lastStability;
    } sleepState;
    
    /**
     * @brief Сохранить состояние в RTC память
     */
    void saveStateToRTC();
    
    /**
     * @brief Загрузить состояние из RTC памяти
     */
    void loadStateFromRTC();
    
    /**
     * @brief Сохранить состояние в Preferences
     */
    void saveStateToPreferences();
    
    /**
     * @brief Загрузить состояние из Preferences
     */
    void loadStateFromPreferences();

public:
    /**
     * @brief Конструктор
     */
    PowerManager();
    
    /**
     * @brief Инициализация менеджера питания
     * @return true если успешно
     */
    bool begin();
    
    /**
     * @brief Включить/выключить Deep Sleep режим
     */
    void setDeepSleepEnabled(bool enabled);
    
    /**
     * @brief Проверить, включен ли Deep Sleep
     */
    bool isDeepSleepEnabled() const { return deepSleepEnabled; }
    
    /**
     * @brief Установить интервал сна
     * @param intervalMs Интервал в миллисекундах
     */
    void setSleepInterval(unsigned long intervalMs);
    
    /**
     * @brief Получить интервал сна
     */
    unsigned long getSleepInterval() const { return sleepIntervalMs; }
    
    /**
     * @brief Перейти в Deep Sleep режим
     * @param durationMs Длительность сна в миллисекундах (0 = использовать установленный интервал)
     */
    void enterDeepSleep(unsigned long durationMs = 0);
    
    /**
     * @brief Проверить, было ли пробуждение из Deep Sleep
     * @return true если пробуждение из Deep Sleep
     */
    bool wasWakeFromDeepSleep();
    
    /**
     * @brief Получить причину пробуждения
     */
    esp_sleep_wakeup_cause_t getWakeupCause();
    
    /**
     * @brief Сохранить текущее состояние перед сном
     * @param alcohol Текущая крепость
     * @param temperature Текущая температура
     * @param stability Текущая стабильность
     */
    void saveCurrentState(float alcohol, float temperature, uint8_t stability);
    
    /**
     * @brief Получить сохраненное состояние
     */
    SleepState getSavedState() const { return sleepState; }
    
    /**
     * @brief Получить количество пробуждений
     */
    uint32_t getWakeCount() const { return sleepState.wakeCount; }
    
    /**
     * @brief Получить общее время в режиме сна (секунды)
     */
    uint32_t getTotalSleepTime() const { return sleepState.totalSleepTime; }
    
    /**
     * @brief Настроить пробуждение по GPIO
     * @param gpioNum Номер GPIO пина
     * @param level Уровень для пробуждения (0 = LOW, 1 = HIGH)
     */
    void enableWakeupOnGPIO(gpio_num_t gpioNum, int level);
    
    /**
     * @brief Отключить пробуждение по GPIO
     */
    void disableWakeupOnGPIO();
};

#endif // POWER_MANAGER_H

