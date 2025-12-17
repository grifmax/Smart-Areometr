#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "config.h"

/**
 * @brief Класс для управления OLED дисплеем
 *
 * Отображает результаты измерений, температуру, статус калибровки
 * и другую информацию на дисплее SSD1306
 */
class DisplayManager {
private:
    Adafruit_SSD1306 *display;
    uint8_t width;
    uint8_t height;

    enum DisplayMode {
        MODE_MEASUREMENT,
        MODE_CALIBRATION,
        MODE_SETTINGS,
        MODE_ERROR
    };

    DisplayMode currentMode;
    String lastError;

public:
    /**
     * @brief Конструктор
     * @param w Ширина дисплея в пикселях
     * @param h Высота дисплея в пикселях
     * @param addr I2C адрес дисплея
     */
    DisplayManager(uint8_t w = OLED_WIDTH, uint8_t h = OLED_HEIGHT, uint8_t addr = OLED_ADDR);

    /**
     * @brief Деструктор
     */
    ~DisplayManager();

    /**
     * @brief Инициализация дисплея
     * @return true если дисплей успешно инициализирован
     */
    bool begin();

    /**
     * @brief Очистить дисплей
     */
    void clear();

    /**
     * @brief Показать экран загрузки
     */
    void showBootScreen();

    /**
     * @brief Показать результаты измерения
     * @param alcoholPercent Процент алкоголя
     * @param temperature Температура
     * @param isCompensated Применена ли температурная компенсация
     */
    void showMeasurement(float alcoholPercent, float temperature, bool isCompensated = false);

    /**
     * @brief Показать экран калибровки
     * @param step Шаг калибровки (0 - вода, 1 - спирт)
     * @param value Текущее значение датчика
     */
    void showCalibration(uint8_t step, uint16_t value);

    /**
     * @brief Показать настройки сети
     * @param ssid SSID сети
     * @param ip IP адрес
     * @param connected Статус подключения
     */
    void showNetworkInfo(const String &ssid, const String &ip, bool connected);

    /**
     * @brief Показать сообщение об ошибке
     * @param error Текст ошибки
     */
    void showError(const String &error);

    /**
     * @brief Показать произвольное сообщение
     * @param message Текст сообщения
     * @param duration Длительность отображения (мс), 0 - бесконечно
     */
    void showMessage(const String &message, uint16_t duration = 0);

    /**
     * @brief Обновить дисплей (вызывать в loop)
     */
    void update();

    /**
     * @brief Установить яркость (если поддерживается)
     * @param brightness Яркость (0-255)
     */
    void setBrightness(uint8_t brightness);
};

#endif // DISPLAY_MANAGER_H
