#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>  // Поддерживаемый форк ESPAsyncWebServer
#include <AsyncTCP.h>
#include <FS.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <ArduinoOTA.h>
#include "config.h"  // Нужно для WEB_SERVER_PORT, DEFAULT_SSID, DEFAULT_PASSWORD

/**
 * @brief Класс для управления веб-сервером и Wi-Fi подключением
 *
 * Предоставляет веб-интерфейс для управления устройством,
 * API для получения данных измерений, OTA обновления
 */
class WebServerManager {
private:
    AsyncWebServer *server;  // Используем AsyncWebServer (поддерживаемый форк)
    String ssid;
    String password;
    bool apMode;  // Режим точки доступа
    String deviceIP;

    // Callback функции для получения данных от основной программы
    std::function<float()> alcoholCallback;
    std::function<float()> temperatureCallback;
    std::function<bool()> calibratedCallback;
    std::function<uint16_t()> rawValueCallback;
    std::function<uint8_t()> stabilityCallback;

    // Callback функции для управления калибровкой
    std::function<bool(float, float)> addCalibrationPointCallback;  // (alcoholPercent, temperature) -> success
    std::function<bool(uint8_t)> deleteCalibrationPointCallback;    // (index) -> success
    std::function<void()> clearCalibrationCallback;
    std::function<String()> getCalibrationDataCallback;            // -> JSON с точками

    // Callback функции для работы с FractionDetector
    std::function<String()> getFractionStatusCallback;              // -> JSON с текущим статусом фракций
    std::function<String()> getFractionStatsCallback;               // -> JSON со статистикой фракций
    std::function<String()> getFractionThresholdsCallback;          // -> JSON с порогами
    std::function<bool(const String&)> setFractionThresholdsCallback; // (JSON) -> success
    std::function<void()> resetFractionSessionCallback;             // Сброс сессии
    std::function<String()> getFractionModeCallback;                // -> режим работы
    std::function<bool(const String&)> setFractionModeCallback;     // (mode) -> success

    /**
     * @brief Настроить маршруты веб-сервера
     */
    void setupRoutes();

    /**
     * @brief Настроить OTA обновления
     */
    void setupOTA();

    /**
     * @brief Генерация HTML главной страницы
     */
    String generateHomePage();

    /**
     * @brief Генерация JSON с данными измерений
     */
    String generateMeasurementJSON();

public:
    /**
     * @brief Конструктор
     * @param port Порт веб-сервера
     */
    WebServerManager(uint16_t port = WEB_SERVER_PORT);

    /**
     * @brief Деструктор
     */
    ~WebServerManager();

    /**
     * @brief Запуск в режиме клиента Wi-Fi
     * @param ssid SSID сети
     * @param password Пароль сети
     * @return true если успешно подключено
     */
    bool beginClient(const String &ssid, const String &password);

    /**
     * @brief Запуск в режиме точки доступа
     * @param ssid SSID точки доступа
     * @param password Пароль (минимум 8 символов)
     * @return true если успешно создана
     */
    bool beginAP(const String &ssid = DEFAULT_SSID, const String &password = DEFAULT_PASSWORD);

    /**
     * @brief Получить IP адрес
     */
    String getIP() const;

    /**
     * @brief Получить SSID
     */
    String getSSID() const;

    /**
     * @brief Проверка подключения к Wi-Fi
     */
    bool isConnected() const;

    /**
     * @brief Проверка режима AP
     */
    bool isAPMode() const;

    /**
     * @brief Установить callback для получения процента алкоголя
     */
    void setAlcoholCallback(std::function<float()> callback);

    /**
     * @brief Установить callback для получения температуры
     */
    void setTemperatureCallback(std::function<float()> callback);

    /**
     * @brief Установить callback для проверки калибровки
     */
    void setCalibratedCallback(std::function<bool()> callback);

    /**
     * @brief Установить callback для получения сырого значения
     */
    void setRawValueCallback(std::function<uint16_t()> callback);

    /**
     * @brief Установить callback для получения стабильности
     */
    void setStabilityCallback(std::function<uint8_t()> callback);

    /**
     * @brief Установить callback для добавления калибровочной точки
     */
    void setAddCalibrationPointCallback(std::function<bool(float, float)> callback);

    /**
     * @brief Установить callback для удаления калибровочной точки
     */
    void setDeleteCalibrationPointCallback(std::function<bool(uint8_t)> callback);

    /**
     * @brief Установить callback для очистки калибровки
     */
    void setClearCalibrationCallback(std::function<void()> callback);

    /**
     * @brief Установить callback для получения данных калибровки
     */
    void setGetCalibrationDataCallback(std::function<String()> callback);

    /**
     * @brief Установить callback для получения статуса фракций
     */
    void setGetFractionStatusCallback(std::function<String()> callback);

    /**
     * @brief Установить callback для получения статистики фракций
     */
    void setGetFractionStatsCallback(std::function<String()> callback);

    /**
     * @brief Установить callback для получения порогов фракций
     */
    void setGetFractionThresholdsCallback(std::function<String()> callback);

    /**
     * @brief Установить callback для установки порогов фракций
     */
    void setSetFractionThresholdsCallback(std::function<bool(const String&)> callback);

    /**
     * @brief Установить callback для сброса сессии фракций
     */
    void setResetFractionSessionCallback(std::function<void()> callback);

    /**
     * @brief Установить callback для получения режима работы
     */
    void setGetFractionModeCallback(std::function<String()> callback);

    /**
     * @brief Установить callback для установки режима работы
     */
    void setSetFractionModeCallback(std::function<bool(const String&)> callback);

    /**
     * @brief Обработка OTA и веб-сервера (вызывать в loop)
     */
    void handle();
    
    /**
     * @brief Обработка OTA (вызывать в loop) - для обратной совместимости
     */
    void handleOTA();

    /**
     * @brief Остановить сервер
     */
    void stop();
};

#endif // WEB_SERVER_H
