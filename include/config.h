#ifndef CONFIG_H
#define CONFIG_H

// Версия прошивки
#define FIRMWARE_VERSION "2.1.6"

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

// === ADS1115 Configuration (16-битный внешний АЦП) ===
#define USE_ADS1115 true              // Включить поддержку ADS1115
#define ADS1115_I2C_ADDRESS 0x48     // Адрес I2C (0x48 по умолчанию, можно 0x49, 0x4A, 0x4B)
// Примечание: GAIN_FOUR будет определен после включения ADS1115Driver.h
// Используем значение 2 для GAIN_FOUR (см. Adafruit_ADS1X15.h)
#define ADS1115_GAIN_VALUE 2         // GAIN_FOUR = 2 (усиление 4×, диапазон ±1.024V)
#define ADS1115_DATA_RATE 128        // Скорость выборки: 8, 16, 32, 64, 128, 250, 475, 860 SPS
#define ADS1115_CHANNEL_0 0           // Канал для электрода 1 (AIN0)
#define ADS1115_CHANNEL_1 1           // Канал для электрода 2 (AIN1)

// Пин емкостного датчика (используется только если USE_ADS1115 = false)
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

// === Battery Monitor Configuration ===
#define BATTERY_MONITOR_ENABLED true
#define BATTERY_ADC_PIN GPIO_NUM_1  // Аналоговый пин для измерения напряжения батареи
#define BATTERY_VOLTAGE_MIN 3.0f    // Минимальное напряжение (В) - полный разряд
#define BATTERY_VOLTAGE_MAX 4.2f    // Максимальное напряжение (В) - полный заряд
#define BATTERY_LOW_THRESHOLD 20    // Порог низкого заряда (%)
#define BATTERY_CRITICAL_THRESHOLD 10  // Порог критического заряда (%)
#define BATTERY_UPDATE_INTERVAL 30000  // Интервал обновления (мс) - 30 секунд
#define BATTERY_DIVIDER_RATIO 2.0f  // Коэффициент делителя напряжения (если используется)

// === Receiver Level Detection (ADS1115) ===
#define RECEIVER_LEVEL_DETECTION_ENABLED true
#define ADS1115_LEVEL_CHANNEL 2  // AIN2 для детекции уровня жидкости
#define LEVEL_DETECTION_THRESHOLD 0.5f  // Пороговое напряжение (В) для срабатывания переполнения
#define LEVEL_DETECTION_DEBOUNCE_MS 1000  // Задержка перед срабатыванием (мс) для устранения ложных срабатываний
#define LEVEL_CHECK_INTERVAL 5000  // Интервал проверки уровня (мс)
#define LEVEL_FILTER_SIZE 5  // Размер фильтра скользящего среднего для стабилизации показаний

// === Receiver Control Configuration ===
#define RECEIVER_CONTROL_ENABLED true
#define RECEIVER_COUNT 3  // Количество приемников (головы, тело, хвосты)
#define RECEIVER_1_PIN GPIO_NUM_7  // GPIO для управления приемником 1 (головы)
#define RECEIVER_2_PIN GPIO_NUM_8  // GPIO для управления приемником 2 (тело)
#define RECEIVER_3_PIN GPIO_NUM_9  // GPIO для управления приемником 3 (хвосты)
#define RECEIVER_MAX_VOLUME_ML 1000.0f  // Максимальный объем приемника (мл)

#endif // CONFIG_H
