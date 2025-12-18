#include <Arduino.h>
#include "SerialCompat.h"
#include "config.h"
#include "ReceiverController.h"
#include "LevelDetector.h"

ReceiverController::ReceiverController()
    : activeReceiverId(0), levelDetector(nullptr),
      overflowAction(OverflowAction::SWITCH_NEXT),
      enabled(RECEIVER_CONTROL_ENABLED),
      autoSwitchEnabled(true) {
    
    // Инициализация приемников с дефолтными значениями
    receivers[0].id = 0;
    receivers[0].name = "Головы";
    receivers[0].controlPin = (gpio_num_t)RECEIVER_1_PIN;
    receivers[0].associatedFraction = Fraction::HEADS;
    
    receivers[1].id = 1;
    receivers[1].name = "Тело";
    receivers[1].controlPin = (gpio_num_t)RECEIVER_2_PIN;
    receivers[1].associatedFraction = Fraction::BODY;
    
    receivers[2].id = 2;
    receivers[2].name = "Хвосты";
    receivers[2].controlPin = (gpio_num_t)RECEIVER_3_PIN;
    receivers[2].associatedFraction = Fraction::TAILS;
    
    // Установка максимального объема
    for (uint8_t i = 0; i < RECEIVER_COUNT; i++) {
        receivers[i].maxVolume = RECEIVER_MAX_VOLUME_ML;
    }
}

bool ReceiverController::begin(LevelDetector* levelDet) {
    levelDetector = levelDet;
    
    // Инициализация GPIO пинов
    for (uint8_t i = 0; i < RECEIVER_COUNT; i++) {
        if (receivers[i].controlPin != GPIO_NUM_MAX) {
            pinMode((int)receivers[i].controlPin, OUTPUT);
            digitalWrite((int)receivers[i].controlPin, LOW);
            receivers[i].isActive = false;
        }
    }
    
    // Деактивируем все приемники при старте
    deactivateAll();
    
    Serial.println("ReceiverController: Initialized");
    Serial.print("  Receivers count: ");
    Serial.println(RECEIVER_COUNT);
    Serial.print("  Auto-switch enabled: ");
    Serial.println(autoSwitchEnabled ? "YES" : "NO");
    
    return true;
}

void ReceiverController::handle() {
    if (!enabled) {
        return;
    }
    
    // Проверка переполнения активного приемника
    if (levelDetector && levelDetector->isEnabled()) {
        bool overflow = checkOverflow();
        
        if (overflow) {
            Serial.print("ReceiverController: Overflow detected on receiver ");
            Serial.println(activeReceiverId);
            
            // Выполняем действие в зависимости от настройки
            switch (overflowAction) {
                case OverflowAction::SWITCH_NEXT:
                    // Переключаемся на следующий приемник
                    if (activeReceiverId < RECEIVER_COUNT - 1) {
                        switchReceiver(activeReceiverId + 1);
                    } else {
                        Serial.println("ReceiverController: All receivers full, stopping");
                        // Можно добавить остановку процесса
                    }
                    break;
                    
                case OverflowAction::STOP:
                    Serial.println("ReceiverController: Overflow - stopping process");
                    deactivateAll();
                    break;
                    
                case OverflowAction::NOTIFY_ONLY:
                    // Только уведомление через callback
                    break;
            }
            
            if (onOverflowCallback) {
                onOverflowCallback(activeReceiverId);
            }
        }
    }
}

void ReceiverController::deactivateAll() {
    for (uint8_t i = 0; i < RECEIVER_COUNT; i++) {
        setReceiverPin(i, false);
        receivers[i].isActive = false;
    }
    activeReceiverId = 255;  // Нет активного приемника
}

void ReceiverController::setReceiverPin(uint8_t receiverId, bool state) {
    if (receiverId >= RECEIVER_COUNT) {
        return;
    }
    
    if (receivers[receiverId].controlPin != GPIO_NUM_MAX) {
        digitalWrite((int)receivers[receiverId].controlPin, state ? HIGH : LOW);
    }
}

bool ReceiverController::switchReceiver(uint8_t receiverId) {
    if (receiverId >= RECEIVER_COUNT) {
        Serial.print("ReceiverController: Invalid receiver ID: ");
        Serial.println(receiverId);
        return false;
    }
    
    // Деактивируем текущий активный приемник
    if (activeReceiverId < RECEIVER_COUNT && receivers[activeReceiverId].isActive) {
        setReceiverPin(activeReceiverId, false);
        receivers[activeReceiverId].isActive = false;
    }
    
    // Активируем новый приемник
    activeReceiverId = receiverId;
    setReceiverPin(receiverId, true);
    receivers[receiverId].isActive = true;
    
    // Сбрасываем флаг переполнения для нового приемника
    receivers[receiverId].isOverflowing = false;
    
    Serial.print("ReceiverController: Switched to receiver ");
    Serial.print(receiverId);
    Serial.print(" (");
    Serial.print(receivers[receiverId].name);
    Serial.println(")");
    
    if (onReceiverSwitchCallback) {
        onReceiverSwitchCallback(receiverId);
    }
    
    return true;
}

bool ReceiverController::switchReceiverByFraction(Fraction fraction) {
    if (!autoSwitchEnabled) {
        return false;
    }
    
    // Находим приемник, ассоциированный с этой фракцией
    for (uint8_t i = 0; i < RECEIVER_COUNT; i++) {
        if (receivers[i].associatedFraction == fraction) {
            return switchReceiver(i);
        }
    }
    
    Serial.print("ReceiverController: No receiver configured for fraction ");
    Serial.println((int)fraction);
    return false;
}

const Receiver& ReceiverController::getReceiver(uint8_t receiverId) const {
    static Receiver emptyReceiver;
    
    if (receiverId >= RECEIVER_COUNT) {
        return emptyReceiver;
    }
    
    return receivers[receiverId];
}

bool ReceiverController::checkOverflow() {
    if (!levelDetector || !levelDetector->isEnabled()) {
        return false;
    }
    
    bool overflow = levelDetector->isOverflow();
    
    // Обновляем состояние переполнения для активного приемника
    if (activeReceiverId < RECEIVER_COUNT) {
        receivers[activeReceiverId].isOverflowing = overflow;
    }
    
    return overflow;
}

void ReceiverController::updateVolume(float flowRate, unsigned long deltaTime) {
    if (activeReceiverId >= RECEIVER_COUNT || !receivers[activeReceiverId].isActive) {
        return;
    }
    
    // Вычисляем приращение объема (flowRate в мл/сек, deltaTime в мс)
    float deltaVolume = flowRate * (deltaTime / 1000.0f);
    receivers[activeReceiverId].currentVolume += deltaVolume;
    
    // Ограничиваем максимальным объемом
    if (receivers[activeReceiverId].currentVolume > receivers[activeReceiverId].maxVolume) {
        receivers[activeReceiverId].currentVolume = receivers[activeReceiverId].maxVolume;
    }
}

void ReceiverController::resetVolume(uint8_t receiverId) {
    if (receiverId >= RECEIVER_COUNT) {
        return;
    }
    
    receivers[receiverId].currentVolume = 0.0f;
    receivers[receiverId].isOverflowing = false;
}

void ReceiverController::configureReceiver(uint8_t receiverId, const String& name,
                                           gpio_num_t pin, float maxVolume, Fraction fraction) {
    if (receiverId >= RECEIVER_COUNT) {
        return;
    }
    
    receivers[receiverId].name = name;
    receivers[receiverId].controlPin = pin;
    receivers[receiverId].maxVolume = maxVolume;
    receivers[receiverId].associatedFraction = fraction;
    
    // Инициализируем GPIO пин
    if (pin != GPIO_NUM_MAX) {
        pinMode((int)pin, OUTPUT);
        digitalWrite((int)pin, LOW);
    }
}
