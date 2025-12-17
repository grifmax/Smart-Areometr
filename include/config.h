#ifndef CONFIG_H
#define CONFIG_H

// Версия прошивки
#define FIRMWARE_VERSION "1.0.0"

// Пины подключения OLED дисплея (I2C)
#define OLED_SDA 8
#define OLED_SCL 9
#define OLED_WIDTH 128
#define OLED_HEIGHT 64
#define OLED_ADDR 0x3C

// Пин емкостного датчика (используем touch pin ESP32-C3)
#define CAPACITIVE_SENSOR_PIN 2  // GPIO2 поддерживает touch

// Пин температурного датчика DS18B20 (OneWire)
#define TEMP_SENSOR_PIN 10

// Кнопки управления
#define BUTTON_CALIBRATE 3
#define BUTTON_MEASURE 4

// Wi-Fi настройки по умолчанию
#define DEFAULT_SSID "Areometr_AP"
#define DEFAULT_PASSWORD "12345678"

// Веб-сервер
#define WEB_SERVER_PORT 80

// Параметры измерения
#define MEASUREMENT_SAMPLES 100  // Количество выборок для усреднения
#define MEASUREMENT_DELAY 10     // Задержка между выборками (мс)

// Калибровочные константы (будут обновляться при калибровке)
#define CALIBRATION_WATER_DEFAULT 0.0f
#define CALIBRATION_ALCOHOL_DEFAULT 100.0f

// Температурная компенсация
#define TEMP_REFERENCE 20.0f  // Эталонная температура (°C)
#define TEMP_COEFFICIENT 0.4f // Коэффициент температурной компенсации (%/°C)

// Логирование
#define MAX_LOG_ENTRIES 100
#define LOG_FILE "/logs.json"

// OTA обновления
#define OTA_HOSTNAME "areometr"
#define OTA_PASSWORD "admin"

// === MQTT Configuration ===
#define MQTT_ENABLED false               // По умолчанию выключено
#define MQTT_SERVER "192.168.1.100"      // Адрес MQTT broker
#define MQTT_PORT 1883                   // Порт MQTT
#define MQTT_USER ""                     // Имя пользователя (пусто = без авторизации)
#define MQTT_PASSWORD ""                 // Пароль
#define MQTT_CLIENT_ID "smart-areometr"  // ID клиента
#define MQTT_RECONNECT_INTERVAL 5000     // Интервал переподключения (мс)
#define MQTT_PUBLISH_INTERVAL 5000       // Интервал публикации данных (мс)

// MQTT топики (базовый топик + подтопики)
#define MQTT_BASE_TOPIC "distillery/areometer"
#define MQTT_TOPIC_STATE "state"
#define MQTT_TOPIC_ALCOHOL "alcohol"
#define MQTT_TOPIC_TEMPERATURE "temperature"
#define MQTT_TOPIC_FRACTION "fraction"
#define MQTT_TOPIC_STABILITY "stability"
#define MQTT_TOPIC_AVAILABILITY "availability"

// MQTT команды (подписка)
#define MQTT_TOPIC_COMMAND "command"
#define MQTT_TOPIC_CALIBRATE "calibrate"
#define MQTT_TOPIC_SET_THRESHOLDS "set_thresholds"

// Home Assistant Discovery
#define MQTT_HA_DISCOVERY_ENABLED true
#define MQTT_HA_DISCOVERY_PREFIX "homeassistant"

#endif // CONFIG_H
