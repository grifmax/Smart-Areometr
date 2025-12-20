#include <Arduino.h>
#include "SerialCompat.h"
#include "PowerManager.h"
#include "config.h"
#include <driver/gpio.h>

// RTC память для сохранения состояния
RTC_DATA_ATTR static uint32_t rtcWakeCount = 0;
RTC_DATA_ATTR static uint32_t rtcTotalSleepTime = 0;
RTC_DATA_ATTR static float rtcLastAlcohol = 0.0f;
RTC_DATA_ATTR static float rtcLastTemperature = 20.0f;
RTC_DATA_ATTR static uint8_t rtcLastStability = 0;

PowerManager::PowerManager()
    : deepSleepEnabled(false), sleepIntervalMs(60000), lastWakeTime(0) {
    sleepState.wakeCount = 0;
    sleepState.totalSleepTime = 0;
    sleepState.lastAlcohol = 0.0f;
    sleepState.lastTemperature = 20.0f;
    sleepState.lastStability = 0;
}

bool PowerManager::begin() {
    preferences.begin("power", false);
    
    // Загружаем настройки из Preferences
    loadStateFromPreferences();
    
    // Загружаем состояние из RTC памяти
    loadStateFromRTC();
    
    // Проверяем причину пробуждения
    if (wasWakeFromDeepSleep()) {
        Serial.println("PowerManager: Woke from Deep Sleep");
        Serial.printf("  Wake count: %lu\n", sleepState.wakeCount);
        Serial.printf("  Total sleep time: %lu seconds\n", sleepState.totalSleepTime);
        
        esp_sleep_wakeup_cause_t cause = getWakeupCause();
        switch (cause) {
            case ESP_SLEEP_WAKEUP_EXT0:
                Serial.println("  Wakeup cause: External signal (RTC_IO)");
                break;
            case ESP_SLEEP_WAKEUP_EXT1:
                Serial.println("  Wakeup cause: External signal (RTC_CNTL)");
                break;
            case ESP_SLEEP_WAKEUP_TIMER:
                Serial.println("  Wakeup cause: Timer");
                break;
            case ESP_SLEEP_WAKEUP_TOUCHPAD:
                Serial.println("  Wakeup cause: Touchpad");
                break;
            case ESP_SLEEP_WAKEUP_ULP:
                Serial.println("  Wakeup cause: ULP program");
                break;
            default:
                Serial.println("  Wakeup cause: Unknown");
                break;
        }
    } else {
        Serial.println("PowerManager: Normal boot (not from Deep Sleep)");
    }
    
    lastWakeTime = millis();
    return true;
}

void PowerManager::setDeepSleepEnabled(bool enabled) {
    deepSleepEnabled = enabled;
    preferences.putBool("deep_sleep_enabled", enabled);
    Serial.printf("PowerManager: Deep Sleep %s\n", enabled ? "enabled" : "disabled");
}

void PowerManager::setSleepInterval(unsigned long intervalMs) {
    sleepIntervalMs = intervalMs;
    preferences.putULong("sleep_interval", intervalMs);
    Serial.printf("PowerManager: Sleep interval set to %lu ms\n", intervalMs);
}

void PowerManager::enterDeepSleep(unsigned long durationMs) {
    if (!deepSleepEnabled) {
        return;
    }
    
    unsigned long sleepDuration = (durationMs > 0) ? durationMs : sleepIntervalMs;
    
    Serial.printf("PowerManager: Entering Deep Sleep for %lu ms\n", sleepDuration);
    Serial.flush();
    
    // Сохраняем состояние перед сном
    saveStateToRTC();
    saveStateToPreferences();
    
    // Настраиваем таймер пробуждения
    esp_sleep_enable_timer_wakeup(sleepDuration * 1000ULL);  // Конвертируем в микросекунды
    
    // Переходим в Deep Sleep
    esp_deep_sleep_start();
    
    // Этот код никогда не выполнится
}

bool PowerManager::wasWakeFromDeepSleep() {
    return esp_sleep_get_wakeup_cause() != ESP_SLEEP_WAKEUP_UNDEFINED;
}

esp_sleep_wakeup_cause_t PowerManager::getWakeupCause() {
    return esp_sleep_get_wakeup_cause();
}

void PowerManager::saveCurrentState(float alcohol, float temperature, uint8_t stability) {
    sleepState.lastAlcohol = alcohol;
    sleepState.lastTemperature = temperature;
    sleepState.lastStability = stability;
}

void PowerManager::saveStateToRTC() {
    rtcWakeCount = sleepState.wakeCount;
    rtcTotalSleepTime = sleepState.totalSleepTime;
    rtcLastAlcohol = sleepState.lastAlcohol;
    rtcLastTemperature = sleepState.lastTemperature;
    rtcLastStability = sleepState.lastStability;
}

void PowerManager::loadStateFromRTC() {
    if (wasWakeFromDeepSleep()) {
        sleepState.wakeCount = rtcWakeCount;
        sleepState.totalSleepTime = rtcTotalSleepTime;
        sleepState.lastAlcohol = rtcLastAlcohol;
        sleepState.lastTemperature = rtcLastTemperature;
        sleepState.lastStability = rtcLastStability;
        
        // Увеличиваем счетчик пробуждений
        sleepState.wakeCount++;
        
        // Вычисляем время сна (приблизительно)
        if (sleepIntervalMs > 0) {
            sleepState.totalSleepTime += (sleepIntervalMs / 1000);
        }
    }
}

void PowerManager::saveStateToPreferences() {
    preferences.putULong("wake_count", sleepState.wakeCount);
    preferences.putULong("total_sleep_time", sleepState.totalSleepTime);
    preferences.putFloat("last_alcohol", sleepState.lastAlcohol);
    preferences.putFloat("last_temperature", sleepState.lastTemperature);
    preferences.putUChar("last_stability", sleepState.lastStability);
}

void PowerManager::loadStateFromPreferences() {
    deepSleepEnabled = preferences.getBool("deep_sleep_enabled", false);
    sleepIntervalMs = preferences.getULong("sleep_interval", 60000);
    sleepState.wakeCount = preferences.getULong("wake_count", 0);
    sleepState.totalSleepTime = preferences.getULong("total_sleep_time", 0);
    sleepState.lastAlcohol = preferences.getFloat("last_alcohol", 0.0f);
    sleepState.lastTemperature = preferences.getFloat("last_temperature", 20.0f);
    sleepState.lastStability = preferences.getUChar("last_stability", 0);
}

void PowerManager::enableWakeupOnGPIO(gpio_num_t gpioNum, int level) {
    // ESP32-C3 не поддерживает ext0/ext1 wakeup напрямую
    // Используем gpio_wakeup для ESP32-C3
    gpio_wakeup_enable(gpioNum, level ? GPIO_INTR_HIGH_LEVEL : GPIO_INTR_LOW_LEVEL);
    esp_sleep_enable_gpio_wakeup();
    Serial.printf("PowerManager: Wakeup on GPIO %d (level: %d) enabled\n", gpioNum, level);
}

void PowerManager::disableWakeupOnGPIO() {
    // Отключаем все GPIO wakeup источники
    esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_EXT0);
    esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_EXT1);
    Serial.println("PowerManager: GPIO wakeup disabled");
}

