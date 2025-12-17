#ifndef ARDUINO_SERIAL_H
#define ARDUINO_SERIAL_H

// Глобальный заголовок для обеспечения доступности Serial на ESP32-C3
// Этот файл автоматически включается через build_flags во всех единицах компиляции

#include <Arduino.h>

#if defined(CONFIG_IDF_TARGET_ESP32C3) || defined(CONFIG_IDF_TARGET_ESP32C6)
    // Для ESP32-C3 с USB CDC (ARDUINO_USB_CDC_ON_BOOT=1) Serial определен ядром
    // Включаем USB.h для доступа к Serial - он должен быть определен там
    #include <USB.h>
    // Serial определен в USB.h как объект USBCDC
#elif defined(ARDUINO_ARCH_ESP32)
    // Для обычного ESP32 Serial определен как HardwareSerial
    extern HardwareSerial Serial;
#endif

#endif // ARDUINO_SERIAL_H

