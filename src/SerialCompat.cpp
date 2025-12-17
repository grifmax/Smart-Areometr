#include "SerialCompat.h"

// Определение Serial для ESP32-C3 с USB CDC fallback
// На ESP32-C3 с ARDUINO_USB_CDC_ON_BOOT=1 Serial должен быть определен в ядре как USBCDC
// Не определяем Serial здесь - он должен быть определен ядром
// Этот файл оставлен для возможных будущих исправлений

