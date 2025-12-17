#ifndef CONFIG_H
#define CONFIG_H

// Версия прошивки
#define FIRMWARE_VERSION "1.0.0"

// Пины подключения OLED дисплея (I2C)
// Для модулей ESP32-C3 с встроенным OLED обычно используются GPIO5 и GPIO6
#define OLED_SDA 5
#define OLED_SCL 6
// Размер дисплея: для 0.42" физически 72x40, но контроллер SSD1306 работает как 128x64
// Нужно использовать 128x64 с offset для правильного отображения
// См. https://github.com/peff74/ESP32-C3_OLED
#define OLED_WIDTH 128
#define OLED_HEIGHT 64
#define OLED_OFFSET_X 30  // Смещение по X: (128-72)/2 = 28, округляем до 30
#define OLED_OFFSET_Y 24  // Смещение по Y: увеличено для опускания изображения ниже
#define OLED_PHYSICAL_WIDTH 72   // Физическая ширина дисплея
#define OLED_PHYSICAL_HEIGHT 40  // Физическая высота дисплея
// Адрес I2C: обычно 0x3C, иногда 0x3D
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
