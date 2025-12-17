# Руководство по устранению неполадок

## Проблемы с запуском

### Устройство не включается

**Симптомы:**
- Дисплей не светится
- Нет реакции на кнопки
- Нет активности на Serial Monitor

**Возможные причины и решения:**

1. **Нет питания:**
   - Проверьте USB кабель
   - Попробуйте другой USB порт
   - Проверьте напряжение на пине 3V3 (должно быть 3.3V)

2. **Поврежден микроконтроллер:**
   - Попробуйте загрузить базовую программу (Blink)
   - Проверьте, определяется ли плата компьютером
   - Возможно требуется замена платы

3. **Короткое замыкание:**
   - Отключите все периферийные устройства
   - Проверьте визуально на наличие замыканий
   - Подключайте датчики по одному

---

### Дисплей не работает

**Симптомы:**
- Экран черный/белый
- Артефакты на экране
- Частичное отображение

**Решения:**

1. **Проверка подключения I2C:**
```cpp
// Загрузите тестовый скрипт I2C Scanner
#include <Wire.h>

void setup() {
  Serial.begin(115200);
  Wire.begin(8, 9); // SDA=8, SCL=9

  Serial.println("I2C Scanner");
  byte count = 0;

  for (byte i = 1; i < 127; i++) {
    Wire.beginTransmission(i);
    if (Wire.endTransmission() == 0) {
      Serial.print("Found device at: 0x");
      Serial.println(i, HEX);
      count++;
    }
  }

  Serial.print("Found ");
  Serial.print(count);
  Serial.println(" device(s).");
}

void loop() {}
```

Ожидаемый результат: должен найти устройство на 0x3C

2. **Неверный I2C адрес:**
   - Попробуйте адрес 0x3D вместо 0x3C
   - Измените в `config.h`: `#define OLED_ADDR 0x3D`

3. **Плохой контакт:**
   - Проверьте пайку SDA/SCL
   - Попробуйте подтянуть I2C линии резисторами 4.7кΩ к 3.3V

4. **Поврежден дисплей:**
   - Замените дисплей

---

## Проблемы с датчиками

### Датчик температуры не найден

**Ошибка в Serial Monitor:**
```
WARNING: No temperature sensor found!
Temperature sensor not available
```

**Решения:**

1. **Проверка подключения:**
   - Убедитесь что DS18B20 подключен к GPIO10
   - Проверьте питание (VCC = 3.3V)
   - Проверьте GND

2. **Подтягивающий резистор:**
   - Обязателен резистор 4.7кΩ между DATA и VCC
   - Проверьте его наличие и правильность подключения

3. **Тест датчика:**
```cpp
#include <OneWire.h>
#include <DallasTemperature.h>

OneWire oneWire(10);
DallasTemperature sensors(&oneWire);

void setup() {
  Serial.begin(115200);
  sensors.begin();
  Serial.print("Found ");
  Serial.print(sensors.getDeviceCount());
  Serial.println(" devices.");
}

void loop() {
  sensors.requestTemperatures();
  float temp = sensors.getTempCByIndex(0);
  Serial.print("Temperature: ");
  Serial.println(temp);
  delay(1000);
}
```

4. **Показывает -127°C:**
   - Это ошибка чтения - плохой контакт
   - Проверьте все соединения
   - Замените датчик

---

### Емкостный датчик показывает неправильные значения

**Симптомы:**
- Всегда 0% или 100%
- Случайные значения
- Нет реакции на изменение раствора

**Решения:**

1. **Проверка touch pin:**
```cpp
void setup() {
  Serial.begin(115200);
}

void loop() {
  uint16_t value = touchRead(2);  // GPIO2
  Serial.println(value);
  delay(100);
}
```

Типичные значения:
- В воздухе: 5-20
- В воде: 50-150
- В спирте: 80-200

Если значения не меняются - проблема с датчиком.

2. **Повторная калибровка:**
   - Выполните калибровку заново
   - Убедитесь что используете чистые растворы
   - Промойте датчик между калибровками

3. **Проверка электродов:**
   - Очистите электроды от налета
   - Убедитесь что нет короткого замыкания
   - Проверьте изоляцию соединений

4. **Изменение параметров измерения:**
В `config.h`:
```cpp
#define MEASUREMENT_SAMPLES 200  // Увеличить выборки
#define MEASUREMENT_DELAY 20     // Увеличить задержку
```

---

## Проблемы с Wi-Fi

### Не создается точка доступа

**Симптомы:**
- Нет сети "Areometr_AP" в списке Wi-Fi
- Ошибка в Serial Monitor: "Failed to create AP"

**Решения:**

1. **Проверка антенны:**
   - ESP32-C3 может иметь встроенную или внешнюю антенну
   - Убедитесь что антенна подключена (если внешняя)

2. **Изменение канала:**
```cpp
// В WebServer.cpp, функция beginAP()
WiFi.softAP(ssid.c_str(), password.c_str(), 6);  // Канал 6
```

3. **Проверка длины пароля:**
   - Минимум 8 символов
   - Максимум 63 символа

4. **Перезагрузка:**
   - Перезагрузите ESP32-C3
   - Сбросьте настройки Wi-Fi:
```cpp
WiFi.disconnect(true);
WiFi.mode(WIFI_OFF);
delay(1000);
```

---

### Не подключается к домашней сети

**Симптомы:**
- Ошибка: "Failed to connect to Wi-Fi"
- Постоянные точки в Serial Monitor

**Решения:**

1. **Проверка SSID и пароля:**
   - Убедитесь в правильности написания
   - Проверьте регистр символов
   - Избегайте специальных символов

2. **Проверка частоты сети:**
   - ESP32-C3 работает только на 2.4 GHz
   - Не поддерживается 5 GHz
   - Убедитесь что роутер не настроен только на 5 GHz

3. **Сила сигнала:**
   - Поднесите устройство ближе к роутеру
   - Проверьте уровень сигнала:
```cpp
int32_t rssi = WiFi.RSSI();
Serial.print("Signal strength: ");
Serial.println(rssi); // Хорошо: > -70 dBm
```

4. **Настройки роутера:**
   - Отключите фильтрацию по MAC-адресу
   - Убедитесь что DHCP включен
   - Попробуйте отключить WPA3 (используйте WPA2)

---

## Проблемы с калибровкой

### Калибровка не сохраняется

**Симптомы:**
- После перезагрузки калибровка теряется
- `isCalibrated` всегда `false`

**Решения:**

1. **Проверка LittleFS:**
```cpp
if (!LittleFS.begin(true)) {
  Serial.println("ERROR: LittleFS mount failed");
}

// Проверка наличия файла
if (LittleFS.exists("/calibration.json")) {
  Serial.println("Calibration file exists");
} else {
  Serial.println("Calibration file NOT found");
}
```

2. **Форматирование файловой системы:**
```cpp
LittleFS.format();
LittleFS.begin();
```

3. **Проверка размера Flash:**
   - В `platformio.ini` должен быть указан правильный размер
   - ESP32-C3 обычно имеет 4MB Flash

---

### Некорректные значения после калибровки

**Симптомы:**
- Показания сильно отличаются от ожидаемых
- Отрицательные значения
- Значения > 100%

**Решения:**

1. **Проверка калибровочных растворов:**
   - Используйте дистиллированную воду (не водопроводную!)
   - Используйте чистый спирт 96-99%
   - Убедитесь что растворы не смешались

2. **Правильная процедура:**
   - Промывайте датчик между калибровками
   - Ждите стабилизации показаний (30 секунд)
   - Не касайтесь электродов руками

3. **Проверка значений:**
```cpp
float water, alcohol;
capacitiveSensor.getCalibration(water, alcohol);
Serial.print("Water: ");
Serial.print(water);
Serial.print(" | Alcohol: ");
Serial.println(alcohol);

// Alcohol должен быть меньше Water
// Например: Water=150, Alcohol=80
```

4. **Ручная установка калибровки:**
Если автоматическая не работает:
```cpp
// Измерьте сырые значения вручную
uint16_t waterValue = 150;   // Замените на свои
uint16_t alcoholValue = 80;  // Замените на свои
capacitiveSensor.setCalibration(waterValue, alcoholValue);
```

---

## Проблемы с производительностью

### Медленные измерения

**Симптомы:**
- Задержка между измерениями > 10 секунд
- Устройство "зависает"

**Решения:**

1. **Уменьшение количества выборок:**
В `config.h`:
```cpp
#define MEASUREMENT_SAMPLES 50  // Вместо 100
```

2. **Оптимизация кода:**
   - Отключите Serial.println() в production коде
   - Уменьшите частоту обновления дисплея

3. **Проверка памяти:**
```cpp
Serial.print("Free heap: ");
Serial.println(ESP.getFreeHeap());
```

Если < 50KB - возможна утечка памяти.

---

### Устройство перезагружается

**Симптомы:**
- Спонтанные перезагрузки
- Ошибка: "Guru Meditation Error" или "Brownout detector"
- Ошибка: "Load access fault" при работе веб-сервера

**Решения:**

1. **Проблема питания:**
   - Используйте качественный USB кабель
   - Попробуйте другой источник питания (мин. 500mA)
   - Добавьте конденсатор 100µF на питание

2. **Stack overflow:**
   - Уменьшите размер локальных переменных
   - Проверьте рекурсивные вызовы
   - Для AsyncTCP увеличьте размер стека в `platformio.ini`:
   ```ini
   build_flags =
       -D CONFIG_ASYNC_TCP_STACK_SIZE=16384
   ```

3. **Проблемы с веб-сервером (Guru Meditation Error: Load access fault):**
   - **Решение:** Используйте поддерживаемый форк ESPAsyncWebServer
   - В `platformio.ini` замените библиотеку:
   ```ini
   lib_deps =
       https://github.com/ESP32Async/ESPAsyncWebServer.git
       me-no-dev/AsyncTCP@^1.1.1
   ```
   - Добавьте флаг компиляции для совместимости:
   ```ini
   build_flags =
       -fpermissive
       -D CONFIG_ASYNC_TCP_STACK_SIZE=16384
   ```
   - Старая библиотека `ESPAsyncWebServer-esphome` может вызывать паники на ESP32-C3

4. **Проверка Watchdog:**
```cpp
// Добавьте в loop()
esp_task_wdt_reset();
```

---

## Проблемы с веб-интерфейсом

### Веб-сервер вызывает панику (Guru Meditation Error)

**Симптомы:**
- Ошибка: "Guru Meditation Error: Core 0 panic'ed (Load access fault)"
- Перезагрузка при обращении к веб-интерфейсу
- Ошибка в `AsyncWebServer::_rewriteRequest`

**Причина:**
Старая библиотека `ESPAsyncWebServer-esphome` имеет проблемы совместимости с ESP32-C3 и может вызывать паники при обработке HTTP запросов.

**Решение:**
1. Используйте поддерживаемый форк ESPAsyncWebServer:
   ```ini
   # В platformio.ini
   lib_deps =
       https://github.com/ESP32Async/ESPAsyncWebServer.git
       me-no-dev/AsyncTCP@^1.1.1
   ```

2. Добавьте флаги компиляции:
   ```ini
   build_flags =
       -fpermissive
       -D CONFIG_ASYNC_TCP_STACK_SIZE=16384
   ```

3. Пересоберите проект:
   ```bash
   pio run --target clean
   pio run
   ```

**Примечание:** Форк ESP32Async/ESPAsyncWebServer активно поддерживается сообществом и совместим с ESP32-C3.

---

### Страница не загружается

**Симптомы:**
- Браузер показывает "Cannot connect"
- Timeout при загрузке

**Решения:**

1. **Проверка IP адреса:**
   - Убедитесь что используете правильный IP
   - Посмотрите IP на дисплее устройства
   - Проверьте подключение ping:
```bash
ping 192.168.4.1
```

2. **Проверка firewall:**
   - Отключите временно firewall/антивирус
   - Разрешите подключения к локальной сети

3. **Браузер:**
   - Попробуйте другой браузер
   - Очистите кеш браузера
   - Отключите блокировщики рекламы

---

### API не отвечает

**Симптомы:**
- 404 ошибка на `/api/measurement`
- JSON не парсится

**Решения:**

1. **Проверка endpoints:**
```bash
# Тест подключения
curl http://192.168.4.1/

# Тест API
curl http://192.168.4.1/api/status
curl http://192.168.4.1/api/measurement
```

2. **Проверка Serial Monitor:**
   - Ищите ошибки в логах
   - Проверьте что веб-сервер запущен

3. **Перезапуск сервера:**
   - Перезагрузите устройство
   - Проверьте что `server->begin()` вызывается

---

## Диагностические команды

### Полная диагностика

Добавьте в код функцию диагностики:

```cpp
void printDiagnostics() {
    Serial.println("\n=== DIAGNOSTICS ===");

    // Система
    Serial.print("Firmware: ");
    Serial.println(FIRMWARE_VERSION);
    Serial.print("Free Heap: ");
    Serial.println(ESP.getFreeHeap());
    Serial.print("CPU Freq: ");
    Serial.println(ESP.getCpuFreqMHz());

    // Wi-Fi
    Serial.print("Wi-Fi Status: ");
    Serial.println(WiFi.status());
    Serial.print("SSID: ");
    Serial.println(WiFi.SSID());
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
    Serial.print("RSSI: ");
    Serial.println(WiFi.RSSI());

    // Датчики
    Serial.print("Temperature Sensor: ");
    Serial.println(tempCompensation.isAvailable() ? "OK" : "FAIL");

    float water, alcohol;
    capacitiveSensor.getCalibration(water, alcohol);
    Serial.print("Calibration: Water=");
    Serial.print(water);
    Serial.print(", Alcohol=");
    Serial.println(alcohol);

    // Файловая система
    Serial.print("LittleFS Total: ");
    Serial.println(LittleFS.totalBytes());
    Serial.print("LittleFS Used: ");
    Serial.println(LittleFS.usedBytes());

    Serial.println("===================\n");
}
```

Вызовите в `setup()` или по команде через Serial.

---

## Получение помощи

Если проблема не решена:

1. **Соберите информацию:**
   - Версия прошивки
   - Полный вывод Serial Monitor
   - Фото подключения
   - Точное описание проблемы

2. **Проверьте документацию:**
   - README.md
   - HARDWARE.md
   - API.md

3. **Создайте Issue на GitHub:**
   - Подробно опишите проблему
   - Приложите диагностическую информацию
   - Укажите что уже пробовали

4. **Полезные ресурсы:**
   - [ESP32-C3 Datasheet](https://www.espressif.com/sites/default/files/documentation/esp32-c3_datasheet_en.pdf)
   - [Arduino ESP32 Forum](https://github.com/espressif/arduino-esp32/issues)
   - [PlatformIO Community](https://community.platformio.org/)
