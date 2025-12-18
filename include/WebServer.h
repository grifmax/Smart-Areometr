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
    std::function<String()> ads1115StatusCallback;  // JSON с информацией об ADS1115

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

    // Callback функции для работы с MQTT
    std::function<String()> getMQTTStatusCallback;                 // -> JSON со статусом MQTT
    std::function<String()> getMQTTConfigCallback;                  // -> JSON с конфигурацией MQTT
    std::function<bool(const String&)> setMQTTConfigCallback;        // (JSON) -> success
    std::function<bool()> testMQTTCallback;                          // -> success (тест подключения)

    // Callback функции для работы с сессиями перегонки
    std::function<bool(const String&, float)> startSessionCallback;  // (name, mashVol) -> success
    std::function<void()> stopSessionCallback;                       // Остановить сессию
    std::function<void()> pauseSessionCallback;                      // Пауза/продолжить
    std::function<String()> getSessionStatusCallback;                // -> JSON со статусом сессии
    std::function<String()> exportSessionJSONCallback;              // -> JSON экспорт
    std::function<String()> exportSessionCSVCallback;                // -> CSV экспорт
    std::function<String()> getSessionsListCallback;                // -> JSON список сессий

    // Callback функции для работы с DataLogger
    std::function<String()> exportLogsCSVCallback;                   // -> CSV экспорт логов
    std::function<String(unsigned long, unsigned long)> getLogsDataCallback;  // (startTime, endTime) -> JSON
    std::function<String()> getLogsStatsCallback;                    // -> JSON статистика

    // Callback функции для работы с BatteryMonitor
    std::function<String()> getBatteryStatusCallback;                // -> JSON статус батареи

    // Callback функции для работы с LevelDetector и ReceiverController
    std::function<String()> getLevelStatusCallback;                  // -> JSON статус детектора уровня
    std::function<float()> getLevelVoltageCallback;                  // -> текущее напряжение
    std::function<bool(float)> setLevelThresholdCallback;            // (threshold) -> success
    std::function<void()> calibrateLevelEmptyCallback;               // Калибровка пустого
    std::function<void()> calibrateLevelFullCallback;                // Калибровка полного
    std::function<String()> getReceiverStatusCallback;               // -> JSON статус приемников
    std::function<bool(uint8_t)> switchReceiverCallback;             // (receiverId) -> success
    std::function<bool(const String&)> setOverflowActionCallback;    // (action) -> success

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
     * @brief Установить callback для получения статуса ADS1115
     */
    void setADS1115StatusCallback(std::function<String()> callback);

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
     * @brief Установить callback для получения статуса MQTT
     */
    void setGetMQTTStatusCallback(std::function<String()> callback);

    /**
     * @brief Установить callback для получения конфигурации MQTT
     */
    void setGetMQTTConfigCallback(std::function<String()> callback);

    /**
     * @brief Установить callback для установки конфигурации MQTT
     */
    void setSetMQTTConfigCallback(std::function<bool(const String&)> callback);

    /**
     * @brief Установить callback для теста подключения MQTT
     */
    void setTestMQTTCallback(std::function<bool()> callback);

    /**
     * @brief Установить callback для начала сессии перегонки
     */
    void setStartSessionCallback(std::function<bool(const String&, float)> callback);

    /**
     * @brief Установить callback для остановки сессии
     */
    void setStopSessionCallback(std::function<void()> callback);

    /**
     * @brief Установить callback для паузы сессии
     */
    void setPauseSessionCallback(std::function<void()> callback);

    /**
     * @brief Установить callback для получения статуса сессии
     */
    void setGetSessionStatusCallback(std::function<String()> callback);

    /**
     * @brief Установить callback для экспорта сессии в JSON
     */
    void setExportSessionJSONCallback(std::function<String()> callback);

    /**
     * @brief Установить callback для экспорта сессии в CSV
     */
    void setExportSessionCSVCallback(std::function<String()> callback);

    /**
     * @brief Установить callback для получения списка сессий
     */
    void setGetSessionsListCallback(std::function<String()> callback);

    /**
     * @brief Установить callback для экспорта логов в CSV
     */
    void setExportLogsCSVCallback(std::function<String()> callback);

    /**
     * @brief Установить callback для получения данных логов за период
     */
    void setGetLogsDataCallback(std::function<String(unsigned long, unsigned long)> callback);

    /**
     * @brief Установить callback для получения статистики логов
     */
    void setGetLogsStatsCallback(std::function<String()> callback);

    /**
     * @brief Установить callback для получения статуса батареи
     */
    void setGetBatteryStatusCallback(std::function<String()> callback);

    /**
     * @brief Установить callback для получения статуса детектора уровня
     */
    void setGetLevelStatusCallback(std::function<String()> callback);

    /**
     * @brief Установить callback для получения напряжения уровня
     */
    void setGetLevelVoltageCallback(std::function<float()> callback);

    /**
     * @brief Установить callback для установки порога уровня
     */
    void setSetLevelThresholdCallback(std::function<bool(float)> callback);

    /**
     * @brief Установить callback для калибровки пустого состояния
     */
    void setCalibrateLevelEmptyCallback(std::function<void()> callback);

    /**
     * @brief Установить callback для калибровки полного состояния
     */
    void setCalibrateLevelFullCallback(std::function<void()> callback);

    /**
     * @brief Установить callback для получения статуса приемников
     */
    void setGetReceiverStatusCallback(std::function<String()> callback);

    /**
     * @brief Установить callback для переключения приемника
     */
    void setSwitchReceiverCallback(std::function<bool(uint8_t)> callback);

    /**
     * @brief Установить callback для установки действия при переполнении
     */
    void setSetOverflowActionCallback(std::function<bool(const String&)> callback);

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
