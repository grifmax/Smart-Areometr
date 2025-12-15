#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <ArduinoJson.h>
#include <ArduinoOTA.h>

/**
 * @brief Класс для управления веб-сервером и Wi-Fi подключением
 *
 * Предоставляет веб-интерфейс для управления устройством,
 * API для получения данных измерений, OTA обновления
 */
class WebServerManager {
private:
    AsyncWebServer *server;
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
     * @brief Обработка OTA (вызывать в loop)
     */
    void handleOTA();

    /**
     * @brief Остановить сервер
     */
    void stop();
};

#endif // WEB_SERVER_H
