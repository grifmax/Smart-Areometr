#ifndef MQTT_BRIDGE_H
#define MQTT_BRIDGE_H

#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "config.h"

// Forward declaration
void mqttCallback(char* topic, byte* payload, unsigned int length);

/**
 * @brief Мост для публикации данных в MQTT broker
 *
 * Обеспечивает интеграцию с системами автоматизации через MQTT протокол.
 * Поддерживает автоматическое переподключение, кеширование при потере связи,
 * и Home Assistant MQTT Discovery.
 *
 * Приоритет: P1 (High) - будет реализовано в v2.0.0
 */
class MQTTBridge {
    friend void mqttCallback(char* topic, byte* payload, unsigned int length);

private:
    WiFiClient wifiClient;
    PubSubClient* mqttClient;

    String server;
    uint16_t port;
    String username;
    String password;
    String clientId;

    bool enabled;
    bool connected;
    unsigned long lastReconnectAttempt;

    // Топики
    String topicState;
    String topicAlcohol;
    String topicTemperature;
    String topicFraction;
    String topicStability;
    String topicBatteryVoltage;
    String topicBatteryPercent;
    String topicBatteryStatus;

    // Callbacks для получения данных
    std::function<float()> alcoholCallback;
    std::function<float()> temperatureCallback;
    std::function<uint8_t()> stabilityCallback;
    std::function<String()> fractionCallback;

    /**
     * @brief Попытка переподключения к MQTT broker
     */
    bool reconnect();

    /**
     * @brief Обработка входящих сообщений
     */
    void messageCallback(char* topic, byte* payload, unsigned int length);

    /**
     * @brief Публикация Home Assistant Discovery сообщений
     */
    void publishDiscovery();

public:
    /**
     * @brief Конструктор
     */
    MQTTBridge();

    /**
     * @brief Деструктор
     */
    ~MQTTBridge();

    /**
     * @brief Инициализация MQTT клиента
     * @param server Адрес MQTT broker
     * @param port Порт (по умолчанию 1883)
     * @param user Имя пользователя (опционально)
     * @param pass Пароль (опционально)
     * @return true если успешно
     */
    bool begin(const String &server, uint16_t port = 1883,
               const String &user = "", const String &pass = "");

    /**
     * @brief Включить/выключить MQTT
     */
    void setEnabled(bool enabled);

    /**
     * @brief Проверить, включен ли MQTT
     */
    bool isEnabled() const;

    /**
     * @brief Установить ID клиента
     */
    void setClientId(const String &id);

    /**
     * @brief Установить базовый топик
     */
    void setBaseTopic(const String &base);

    /**
     * @brief Обработка MQTT (вызывать в loop)
     */
    void handle();

    /**
     * @brief Проверка подключения
     */
    bool isConnected() const;

    /**
     * @brief Публикация данных ареометра
     */
    void publishMeasurement(float alcohol, float temperature, uint8_t stability);

    /**
     * @brief Публикация смены фракции
     */
    void publishFractionChange(const String &fraction);

    /**
     * @brief Публикация полного состояния
     */
    void publishState();

    /**
     * @brief Публикация события
     */
    void publishEvent(const String &event, const String &data);

    /**
     * @brief Подписаться на топик команд
     */
    void subscribeToCommands();

    /**
     * @brief Установить callback для получения крепости
     */
    void setAlcoholCallback(std::function<float()> callback);

    /**
     * @brief Установить callback для получения температуры
     */
    void setTemperatureCallback(std::function<float()> callback);

    /**
     * @brief Установить callback для получения стабильности
     */
    void setStabilityCallback(std::function<uint8_t()> callback);

    /**
     * @brief Установить callback для получения фракции
     */
    void setFractionCallback(std::function<String()> callback);

    /**
     * @brief Публикация статуса батареи
     * @param voltage Напряжение батареи (В)
     * @param percent Процент заряда (0-100)
     * @param charging Статус зарядки
     */
    void publishBatteryStatus(float voltage, uint8_t percent, bool charging);
};

#endif // MQTT_BRIDGE_H
