#include "SerialCompat.h"

// Определение Serial для ESP32-C3 с USB CDC
// На ESP32-C3 с ARDUINO_USB_CDC_ON_BOOT=1 Serial должен быть определен в ядре как HWCDC
// Но если ядро не экспортирует Serial глобально, определяем его здесь
#if defined(CONFIG_IDF_TARGET_ESP32C3) || defined(CONFIG_IDF_TARGET_ESP32C6)
    #if ARDUINO_USB_CDC_ON_BOOT
        // Serial должен быть определен в ядре, но если нет - создаем fallback
        // Используем HWCDC для ESP32-C3
        #include <USB.h>
        #include <HWCDC.h>
        // Определяем Serial для ESP32-C3
        // Если Serial уже определен в ядре, линкер выдаст ошибку множественного определения
        // Но раз линкер не может найти Serial, значит он не определен, так что определяем его
        HWCDC Serial;
    #endif
#endif

