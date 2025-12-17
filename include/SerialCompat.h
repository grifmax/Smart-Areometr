#ifndef SERIAL_COMPAT_H
#define SERIAL_COMPAT_H

// Этот заголовок должен включаться только в C++ файлах
#ifdef __cplusplus

#include <Arduino.h>

// Универсальная декларация Serial для ESP32/ESP32-C3.
// Для ESP32-C3 с USB CDC (ARDUINO_USB_CDC_ON_BOOT=1) Serial определен ядром как HWCDC
// Для обычного ESP32 Serial определен как HardwareSerial
#if defined(CONFIG_IDF_TARGET_ESP32C3) || defined(CONFIG_IDF_TARGET_ESP32C6)
    // ESP32-C3 с USB CDC - Serial это HWCDC
    #include <USB.h>
    #include <HWCDC.h>
    // Явная декларация Serial для доступа из наших файлов
    // Serial определен в ядре, extern просто ссылается на это определение
    extern HWCDC Serial;
#elif defined(ARDUINO_ARCH_ESP32)
    // Для обычного ESP32 Serial определен как HardwareSerial
    #include <HardwareSerial.h>
    extern HardwareSerial Serial;
#else
    // Fallback для других платформ
    #include <HardwareSerial.h>
    extern HardwareSerial Serial;
#endif

#endif // __cplusplus

#endif // SERIAL_COMPAT_H


