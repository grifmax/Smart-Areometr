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

#endif // CONFIG_H
