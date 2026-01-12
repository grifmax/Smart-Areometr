# ESP32-C3 Compilation Fixes

## Проблема

ESP32-C3 (RISC-V) не имеет аппаратного модуля для емкостных touch-сенсоров, в отличие от оригинального ESP32. При попытке компиляции кода для ESP32-C3 возникали ошибки:

```
error: 'touchRead' was not declared in this scope
error: 'Serial' was not declared in this scope
```

## Решение в трёх итерациях

### Итерация 1: Попытка с макросом ESP32 (commit: 6994661)

**Подход**: Использовали `#ifdef ESP32` для определения типа чипа

**Проблема**: Оба чипа (ESP32 и ESP32-C3) определяют макрос `ESP32`, поэтому условная компиляция не работала

```cpp
// НЕ РАБОТАЕТ - оба чипа определяют ESP32
#ifdef ESP32
    touchRead(pin);  // ❌ Ошибка на ESP32-C3
#else
    analogRead(pin);
#endif
```

**Другие исправления в этом коммите**:
- Добавлены `#include <Arduino.h>` во все .cpp файлы для использования Serial
- Добавлены `#include "config.h"` в заголовочные файлы для макросов (OLED_WIDTH, TEMP_REFERENCE, LOG_FILE и т.д.)
- Добавлен friend declaration в MQTTBridge.h для доступа к приватному методу messageCallback

### Итерация 2: Попытка с SOC_TOUCH_SENSOR_NUM (commit: 1a89e2b)

**Подход**: Использовали макрос `SOC_TOUCH_SENSOR_NUM` из ESP-IDF

**Проблема**: Макрос `SOC_TOUCH_SENSOR_NUM` может быть недоступен в Arduino framework

```cpp
// НЕ РАБОТАЕТ - макрос может отсутствовать
#if SOC_TOUCH_SENSOR_NUM > 0
    touchRead(pin);  // ❌ SOC_TOUCH_SENSOR_NUM not defined
#else
    analogRead(pin);
#endif
```

### Итерация 3: CONFIG_IDF_TARGET_ESP32C3/C6 (commit: 3841e3e) ✅

**Подход**: Используем явные макросы определения целевого чипа из ESP-IDF/Arduino framework

**Решение**: Проверяем, что чип **НЕ** ESP32-C3 и **НЕ** ESP32-C6 (только эти не имеют touch sensor)

```cpp
// ✅ РАБОТАЕТ - надёжные макросы от ESP-IDF
#if !defined(CONFIG_IDF_TARGET_ESP32C3) && !defined(CONFIG_IDF_TARGET_ESP32C6)
    // Только для ESP32 с аппаратным модулем тачсенсора
    touchRead(pin);
#else
    // Для ESP32-C3/C6 без тачсенсора используем ADC
    pinMode(pin, INPUT);
    analogSetAttenuation(ADC_11db);  // Полный диапазон 0-3.3V
    analogRead(pin);
#endif
```

## Исправленные файлы

### src/CapacitiveSensor.cpp
- `begin()`: Инициализация сенсора с проверкой чипа
- `getRawValue()`: Чтение значений с правильным API

### src/CapacitiveSensorV2.cpp
- `begin()`: Инициализация сенсора с проверкой чипа
- `begin()`: Калибровка baseline с правильным API
- `getRawValue()`: Чтение значений в цикле усреднения

### include/*.h
- DisplayManager.h: Добавлен `#include "config.h"` для OLED_WIDTH, OLED_HEIGHT, OLED_ADDR
- TemperatureCompensation.h: Добавлен `#include "config.h"` для TEMP_REFERENCE, TEMP_COEFFICIENT
- DataLogger.h: Добавлен `#include "config.h"` для LOG_FILE, MAX_LOG_ENTRIES
- MQTTBridge.h: Добавлен friend declaration для mqttCallback

### src/*.cpp (все файлы)
Добавлены `#include <Arduino.h>` для использования Serial, pinMode, delay и других Arduino функций:
- src/CapacitiveSensor.cpp
- src/CapacitiveSensorV2.cpp
- src/FractionDetector.cpp
- src/CalibrationTables.cpp
- src/DataLogger.cpp
- src/MQTTBridge.cpp

## Как проверить компиляцию

### Через PlatformIO

```bash
# Установка PlatformIO (если не установлен)
pip3 install platformio

# Компиляция для ESP32-C3
platformio run -e esp32-c3-devkitm-1

# Загрузка на устройство
platformio run -e esp32-c3-devkitm-1 --target upload

# Мониторинг Serial порта
platformio device monitor -b 115200
```

### Через Arduino IDE

1. Установите ESP32 board package: https://docs.espressif.com/projects/arduino-esp32/en/latest/installing.html
2. Выберите плату: **ESP32C3 Dev Module**
3. Откройте `src/main.cpp`
4. Нажмите **Verify** для компиляции

### Проверка макросов

Чтобы убедиться, что макросы определены правильно, можно добавить отладочный код:

```cpp
void setup() {
    Serial.begin(115200);
    delay(1000);

    #if defined(CONFIG_IDF_TARGET_ESP32C3)
        Serial.println("Running on ESP32-C3 (RISC-V, no touch sensor)");
    #elif defined(CONFIG_IDF_TARGET_ESP32C6)
        Serial.println("Running on ESP32-C6 (RISC-V, no touch sensor)");
    #else
        Serial.println("Running on ESP32 (Xtensa, with touch sensor)");
    #endif
}
```

## Технические детали

### Почему ESP32-C3 не имеет touchRead()?

**ESP32 (оригинальный)**:
- Архитектура: Xtensa LX6
- Touch sensor: Аппаратный модуль для 10 сенсоров
- API: `touchRead(pin)` - встроенная функция

**ESP32-C3**:
- Архитектура: RISC-V 32-bit
- Touch sensor: ❌ Отсутствует
- Альтернатива: ADC (Analog-to-Digital Converter) для ёмкостных измерений
- API: `analogRead(pin)` - стандартный ADC

### Настройка ADC для ESP32-C3

```cpp
pinMode(pin, INPUT);
analogSetAttenuation(ADC_11db);  // Диапазон 0-3.3V
uint16_t value = analogRead(pin);  // Чтение значения
```

**Параметры ADC**:
- Разрешение: 12 бит (0-4095)
- Ослабление: 11 dB (полный диапазон 0-3.3V)
- Частота дискретизации: до 100 kHz

### Калибровка для разных чипов

**Важно**: Калибровочные значения для ESP32 и ESP32-C3 будут **разные**!

- **ESP32**: `touchRead()` возвращает меньшие значения при касании (обратная логика)
- **ESP32-C3**: `analogRead()` возвращает значения пропорционально напряжению (прямая логика)

После перехода с ESP32 на ESP32-C3 (или наоборот) необходимо **пересоздать калибровку**.

## Проверка работы

После успешной компиляции и загрузки проверьте:

1. **Serial Monitor** должен показать:
   ```
   =============================
   Smart Areometr v2.2.0
   =============================

   Capacitive sensor V2 initialized on pin 4
   Calibrating touch sensor baseline...
   Touch sensor baseline: 1234
   ...
   ```

2. **Веб-интерфейс** должен открыться по IP адресу:
   - Страница калибровки: http://192.168.4.1/calibration.html
   - Сырые значения должны обновляться в реальном времени

3. **Измерения** должны работать:
   - В воде: значение ~1000-2000
   - В спирте: значение ~500-1500 (зависит от чипа)

## Проблемы с веб-сервером на ESP32-C3

### Guru Meditation Error при работе веб-сервера

**Проблема:**
При использовании библиотеки `ESPAsyncWebServer-esphome` на ESP32-C3 возникают паники:
```
Guru Meditation Error: Core 0 panic'ed (Load access fault)
Exception was unhandled.
```

**Причина:**
Библиотека `ESPAsyncWebServer-esphome` имеет проблемы совместимости с ESP32-C3 и может вызывать паники при обработке HTTP запросов.

**Решение:**
Используйте поддерживаемый форк ESPAsyncWebServer от сообщества:

```ini
# В platformio.ini
lib_deps =
    https://github.com/ESP32Async/ESPAsyncWebServer.git
    me-no-dev/AsyncTCP@^1.1.1
```

Добавьте флаги компиляции:
```ini
build_flags =
    -fpermissive
    -D CONFIG_ASYNC_TCP_STACK_SIZE=16384
```

**Примечание:** Форк ESP32Async/ESPAsyncWebServer активно поддерживается и совместим с ESP32-C3.

---

## Связанные коммиты

- `04e3da7`: Add dual-mode operation and audio alerts (основная функциональность)
- `6994661`: Fix compilation errors for ESP32-C3 (первая попытка)
- `1a89e2b`: Fix ESP32-C3 detection - use SOC_TOUCH_SENSOR_NUM (вторая попытка)
- `3841e3e`: Fix ESP32-C3 chip detection using CONFIG_IDF_TARGET macros (финальное решение) ✅
- `800a195`: Switch to maintained ESPAsyncWebServer fork for ESP32-C3 compatibility (исправление веб-сервера) ✅

## Поддерживаемые чипы

После этих исправлений проект должен компилироваться для:

- ✅ **ESP32** (Xtensa LX6) - использует touchRead()
- ✅ **ESP32-S2** (Xtensa LX7) - использует touchRead()
- ✅ **ESP32-S3** (Xtensa LX7) - использует touchRead()
- ✅ **ESP32-C3** (RISC-V) - использует analogRead()
- ✅ **ESP32-C6** (RISC-V) - использует analogRead()

## Дополнительные ресурсы

- [ESP32-C3 Technical Reference Manual](https://www.espressif.com/sites/default/files/documentation/esp32-c3_technical_reference_manual_en.pdf)
- [Arduino-ESP32 API Reference](https://docs.espressif.com/projects/arduino-esp32/en/latest/api/adc.html)
- [Conditional Compilation in Arduino](https://github.com/espressif/arduino-esp32/blob/master/cores/esp32/esp32-hal.h)

---

*Документ обновлен: 2025-12-17*
*Финальное решение: commit 3841e3e*
