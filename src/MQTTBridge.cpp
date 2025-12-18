#include <Arduino.h>
#include "SerialCompat.h"
#include "MQTTBridge.h"

// Статическая ссылка на текущий экземпляр для callback
static MQTTBridge* mqttBridgeInstance = nullptr;

// Статический callback для PubSubClient
void mqttCallback(char* topic, byte* payload, unsigned int length) {
    if (mqttBridgeInstance) {
        mqttBridgeInstance->messageCallback(topic, payload, length);
    }
}

MQTTBridge::MQTTBridge()
    : mqttClient(nullptr), server(MQTT_SERVER), port(MQTT_PORT),
      username(MQTT_USER), password(MQTT_PASSWORD), clientId(MQTT_CLIENT_ID),
      enabled(MQTT_ENABLED), connected(false), lastReconnectAttempt(0) {

    // Инициализация топиков
    topicState = String(MQTT_BASE_TOPIC) + "/" + MQTT_TOPIC_STATE;
    topicAlcohol = String(MQTT_BASE_TOPIC) + "/" + MQTT_TOPIC_ALCOHOL;
    topicTemperature = String(MQTT_BASE_TOPIC) + "/" + MQTT_TOPIC_TEMPERATURE;
    topicFraction = String(MQTT_BASE_TOPIC) + "/" + MQTT_TOPIC_FRACTION;
    topicStability = String(MQTT_BASE_TOPIC) + "/" + MQTT_TOPIC_STABILITY;
    topicBatteryVoltage = String(MQTT_BASE_TOPIC) + "/battery/voltage";
    topicBatteryPercent = String(MQTT_BASE_TOPIC) + "/battery/percent";
    topicBatteryStatus = String(MQTT_BASE_TOPIC) + "/battery/status";

    mqttBridgeInstance = this;
}

MQTTBridge::~MQTTBridge() {
    if (mqttClient) {
        if (mqttClient->connected()) {
            // Публикуем offline перед отключением
            String availTopic = String(MQTT_BASE_TOPIC) + "/" + MQTT_TOPIC_AVAILABILITY;
            mqttClient->publish(availTopic.c_str(), "offline", true);
            mqttClient->disconnect();
        }
        delete mqttClient;
    }
    mqttBridgeInstance = nullptr;
}

bool MQTTBridge::begin(const String &s, uint16_t p, const String &user, const String &pass) {
    server = s;
    port = p;
    username = user;
    password = pass;

    if (!enabled) {
        Serial.println("MQTT: Disabled in configuration");
        return false;
    }

    if (server.isEmpty()) {
        Serial.println("MQTT: Server address is empty");
        return false;
    }

    // Создаем клиента если еще не создан
    if (!mqttClient) {
        mqttClient = new PubSubClient(wifiClient);
        mqttClient->setServer(server.c_str(), port);
        mqttClient->setCallback(::mqttCallback);
        mqttClient->setBufferSize(512);  // Увеличиваем буфер для больших сообщений
    }

    Serial.printf("MQTT: Configured for %s:%d\n", server.c_str(), port);
    return true;
}

void MQTTBridge::setEnabled(bool e) {
    enabled = e;
    if (!enabled && mqttClient && mqttClient->connected()) {
        mqttClient->disconnect();
        connected = false;
    }
}

bool MQTTBridge::isEnabled() const {
    return enabled;
}

void MQTTBridge::setClientId(const String &id) {
    clientId = id;
}

void MQTTBridge::setBaseTopic(const String &base) {
    topicState = base + "/" + MQTT_TOPIC_STATE;
    topicAlcohol = base + "/" + MQTT_TOPIC_ALCOHOL;
    topicTemperature = base + "/" + MQTT_TOPIC_TEMPERATURE;
    topicFraction = base + "/" + MQTT_TOPIC_FRACTION;
    topicStability = base + "/" + MQTT_TOPIC_STABILITY;
    topicBatteryVoltage = base + "/battery/voltage";
    topicBatteryPercent = base + "/battery/percent";
    topicBatteryStatus = base + "/battery/status";
}

bool MQTTBridge::reconnect() {
    if (!enabled || !mqttClient) {
        return false;
    }

    Serial.print("MQTT: Attempting connection...");

    // Формируем Last Will and Testament
    String availTopic = String(MQTT_BASE_TOPIC) + "/" + MQTT_TOPIC_AVAILABILITY;

    bool connectResult;
    if (username.isEmpty()) {
        connectResult = mqttClient->connect(
            clientId.c_str(),
            availTopic.c_str(), 0, true, "offline"
        );
    } else {
        connectResult = mqttClient->connect(
            clientId.c_str(),
            username.c_str(),
            password.c_str(),
            availTopic.c_str(), 0, true, "offline"
        );
    }

    if (connectResult) {
        Serial.println(" connected!");
        connected = true;

        // Публикуем online статус
        mqttClient->publish(availTopic.c_str(), "online", true);

        // Подписываемся на команды
        subscribeToCommands();

        // Публикуем Home Assistant Discovery
        if (MQTT_HA_DISCOVERY_ENABLED) {
            publishDiscovery();
        }

        return true;
    } else {
        Serial.print(" failed, rc=");
        Serial.println(mqttClient->state());
        connected = false;
        return false;
    }
}

void MQTTBridge::handle() {
    if (!enabled || !mqttClient) {
        return;
    }

    // Поддержка соединения
    if (mqttClient->connected()) {
        mqttClient->loop();
        connected = true;
    } else {
        connected = false;
        unsigned long now = millis();
        if (now - lastReconnectAttempt > MQTT_RECONNECT_INTERVAL) {
            lastReconnectAttempt = now;
            reconnect();
        }
    }
}

bool MQTTBridge::isConnected() const {
    return connected && mqttClient && mqttClient->connected();
}

void MQTTBridge::publishMeasurement(float alcohol, float temperature, uint8_t stability) {
    if (!isConnected()) return;

    char buffer[32];

    // Публикуем каждый параметр отдельно
    snprintf(buffer, sizeof(buffer), "%.2f", alcohol);
    mqttClient->publish(topicAlcohol.c_str(), buffer, false);

    snprintf(buffer, sizeof(buffer), "%.2f", temperature);
    mqttClient->publish(topicTemperature.c_str(), buffer, false);

    snprintf(buffer, sizeof(buffer), "%d", stability);
    mqttClient->publish(topicStability.c_str(), buffer, false);

    Serial.printf("MQTT: Published - Alcohol: %.2f%%, Temp: %.2f°C, Stability: %d%%\n",
                  alcohol, temperature, stability);
}

void MQTTBridge::publishFractionChange(const String &fraction) {
    if (!isConnected()) return;

    mqttClient->publish(topicFraction.c_str(), fraction.c_str(), true);

    // Публикуем событие
    String eventTopic = String(MQTT_BASE_TOPIC) + "/event";
    String eventData = "{\"event\":\"fraction_change\",\"fraction\":\"" + fraction + "\"}";
    mqttClient->publish(eventTopic.c_str(), eventData.c_str(), false);

    Serial.printf("MQTT: Fraction changed to %s\n", fraction.c_str());
}

void MQTTBridge::publishBatteryStatus(float voltage, uint8_t percent, bool charging) {
    if (!isConnected()) return;

    char buffer[16];

    // Публикуем напряжение
    snprintf(buffer, sizeof(buffer), "%.2f", voltage);
    mqttClient->publish(topicBatteryVoltage.c_str(), buffer, false);

    // Публикуем процент заряда
    snprintf(buffer, sizeof(buffer), "%d", percent);
    mqttClient->publish(topicBatteryPercent.c_str(), buffer, false);

    // Публикуем полный статус (JSON)
    JsonDocument doc;
    doc["voltage"] = voltage;
    doc["percent"] = percent;
    doc["charging"] = charging;
    doc["low_battery"] = percent <= 20;
    doc["critical"] = percent <= 10;

    String json;
    serializeJson(doc, json);
    mqttClient->publish(topicBatteryStatus.c_str(), json.c_str(), false);

    Serial.printf("MQTT: Published battery status - %.2fV, %d%%, charging: %s\n",
                  voltage, percent, charging ? "yes" : "no");
}

void MQTTBridge::publishState() {
    if (!isConnected()) return;

    JsonDocument doc;

    // Собираем все данные через callbacks
    if (alcoholCallback) doc["alcohol"] = alcoholCallback();
    if (temperatureCallback) doc["temperature"] = temperatureCallback();
    if (stabilityCallback) doc["stability"] = stabilityCallback();
    if (fractionCallback) doc["fraction"] = fractionCallback();

    doc["timestamp"] = millis();
    doc["uptime"] = millis() / 1000;

    String json;
    serializeJson(doc, json);

    mqttClient->publish(topicState.c_str(), json.c_str(), false);
    Serial.println("MQTT: State published");
}

void MQTTBridge::publishEvent(const String &event, const String &data) {
    if (!isConnected()) return;

    String eventTopic = String(MQTT_BASE_TOPIC) + "/event";
    String payload = "{\"event\":\"" + event + "\",\"data\":\"" + data + "\"}";

    mqttClient->publish(eventTopic.c_str(), payload.c_str(), false);
    Serial.printf("MQTT: Event published - %s\n", event.c_str());
}

void MQTTBridge::subscribeToCommands() {
    if (!isConnected()) return;

    String commandTopic = String(MQTT_BASE_TOPIC) + "/" + MQTT_TOPIC_COMMAND;
    String calibrateTopic = String(MQTT_BASE_TOPIC) + "/" + MQTT_TOPIC_CALIBRATE;
    String thresholdsTopic = String(MQTT_BASE_TOPIC) + "/" + MQTT_TOPIC_SET_THRESHOLDS;

    mqttClient->subscribe(commandTopic.c_str());
    mqttClient->subscribe(calibrateTopic.c_str());
    mqttClient->subscribe(thresholdsTopic.c_str());

    Serial.println("MQTT: Subscribed to command topics");
}

void MQTTBridge::messageCallback(char* topic, byte* payload, unsigned int length) {
    // Конвертируем payload в строку
    String message;
    for (unsigned int i = 0; i < length; i++) {
        message += (char)payload[i];
    }

    Serial.printf("MQTT: Message received [%s]: %s\n", topic, message.c_str());

    String topicStr = String(topic);

    // Обработка команд
    if (topicStr.endsWith("/" + String(MQTT_TOPIC_COMMAND))) {
        if (message == "calibrate") {
            publishEvent("command_received", "calibrate");
        } else if (message == "reset") {
            publishEvent("command_received", "reset");
        } else if (message == "restart") {
            publishEvent("command_received", "restart");
            delay(1000);
            ESP.restart();
        }
    }
    else if (topicStr.endsWith("/" + String(MQTT_TOPIC_CALIBRATE))) {
        // JSON: {"alcohol": 40.0, "temperature": 20.0}
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, message);
        if (!error) {
            float alcohol = doc["alcohol"] | 0.0f;
            float temp = doc["temperature"] | 20.0f;
            publishEvent("calibration_requested",
                        String("alcohol:") + alcohol + ",temp:" + temp);
        }
    }
    else if (topicStr.endsWith("/" + String(MQTT_TOPIC_SET_THRESHOLDS))) {
        // JSON с порогами фракций
        publishEvent("thresholds_update_requested", message);
    }
}

void MQTTBridge::publishDiscovery() {
    if (!isConnected()) return;

    String deviceId = clientId;
    String deviceName = "Smart Areometr";

    // Device info для всех entities
    String deviceInfo = String(",\"device\":{") +
        "\"identifiers\":[\"" + deviceId + "\"]," +
        "\"name\":\"" + deviceName + "\"," +
        "\"model\":\"ESP32-C3 Areometer\"," +
        "\"manufacturer\":\"DIY\"," +
        "\"sw_version\":\"" + String(FIRMWARE_VERSION) + "\"" +
    "}";

    // 1. Sensor: Alcohol percentage
    String alcoholConfig = String("{") +
        "\"name\":\"Alcohol Percentage\"," +
        "\"unique_id\":\"" + deviceId + "_alcohol\"," +
        "\"state_topic\":\"" + topicAlcohol + "\"," +
        "\"unit_of_measurement\":\"%\"," +
        "\"icon\":\"mdi:bottle-wine\"," +
        "\"state_class\":\"measurement\"" +
        deviceInfo + "}";

    String alcoholTopic = String(MQTT_HA_DISCOVERY_PREFIX) + "/sensor/" + deviceId + "/alcohol/config";
    mqttClient->publish(alcoholTopic.c_str(), alcoholConfig.c_str(), true);

    // 2. Sensor: Temperature
    String tempConfig = String("{") +
        "\"name\":\"Temperature\"," +
        "\"unique_id\":\"" + deviceId + "_temperature\"," +
        "\"state_topic\":\"" + topicTemperature + "\"," +
        "\"unit_of_measurement\":\"°C\"," +
        "\"device_class\":\"temperature\"," +
        "\"state_class\":\"measurement\"" +
        deviceInfo + "}";

    String tempTopic = String(MQTT_HA_DISCOVERY_PREFIX) + "/sensor/" + deviceId + "/temperature/config";
    mqttClient->publish(tempTopic.c_str(), tempConfig.c_str(), true);

    // 3. Sensor: Stability
    String stabilityConfig = String("{") +
        "\"name\":\"Signal Stability\"," +
        "\"unique_id\":\"" + deviceId + "_stability\"," +
        "\"state_topic\":\"" + topicStability + "\"," +
        "\"unit_of_measurement\":\"%\"," +
        "\"icon\":\"mdi:signal\"," +
        "\"state_class\":\"measurement\"" +
        deviceInfo + "}";

    String stabilityTopic = String(MQTT_HA_DISCOVERY_PREFIX) + "/sensor/" + deviceId + "/stability/config";
    mqttClient->publish(stabilityTopic.c_str(), stabilityConfig.c_str(), true);

    // 4. Sensor: Fraction
    String fractionConfig = String("{") +
        "\"name\":\"Distillation Fraction\"," +
        "\"unique_id\":\"" + deviceId + "_fraction\"," +
        "\"state_topic\":\"" + topicFraction + "\"," +
        "\"icon\":\"mdi:beaker\"" +
        deviceInfo + "}";

    String fractionTopic = String(MQTT_HA_DISCOVERY_PREFIX) + "/sensor/" + deviceId + "/fraction/config";
    mqttClient->publish(fractionTopic.c_str(), fractionConfig.c_str(), true);

    // 5. Sensor: Battery voltage
    String batteryVoltageConfig = String("{") +
        "\"name\":\"Battery Voltage\"," +
        "\"unique_id\":\"" + deviceId + "_battery_voltage\"," +
        "\"state_topic\":\"" + topicBatteryVoltage + "\"," +
        "\"unit_of_measurement\":\"V\"," +
        "\"device_class\":\"voltage\"," +
        "\"state_class\":\"measurement\"," +
        "\"icon\":\"mdi:battery\"" +
        deviceInfo + "}";

    String batteryVoltageTopic = String(MQTT_HA_DISCOVERY_PREFIX) + "/sensor/" + deviceId + "/battery_voltage/config";
    mqttClient->publish(batteryVoltageTopic.c_str(), batteryVoltageConfig.c_str(), true);

    // 6. Sensor: Battery percentage
    String batteryPercentConfig = String("{") +
        "\"name\":\"Battery\"," +
        "\"unique_id\":\"" + deviceId + "_battery_percent\"," +
        "\"state_topic\":\"" + topicBatteryPercent + "\"," +
        "\"unit_of_measurement\":\"%\"," +
        "\"device_class\":\"battery\"," +
        "\"state_class\":\"measurement\"," +
        "\"icon\":\"mdi:battery\"" +
        deviceInfo + "}";

    String batteryPercentTopic = String(MQTT_HA_DISCOVERY_PREFIX) + "/sensor/" + deviceId + "/battery_percent/config";
    mqttClient->publish(batteryPercentTopic.c_str(), batteryPercentConfig.c_str(), true);

    Serial.println("MQTT: Home Assistant Discovery published");
}

// Setters для callbacks
void MQTTBridge::setAlcoholCallback(std::function<float()> callback) {
    alcoholCallback = callback;
}

void MQTTBridge::setTemperatureCallback(std::function<float()> callback) {
    temperatureCallback = callback;
}

void MQTTBridge::setStabilityCallback(std::function<uint8_t()> callback) {
    stabilityCallback = callback;
}

void MQTTBridge::setFractionCallback(std::function<String()> callback) {
    fractionCallback = callback;
}
