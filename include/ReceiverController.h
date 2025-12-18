#ifndef RECEIVER_CONTROLLER_H
#define RECEIVER_CONTROLLER_H

#include <Arduino.h>
#include <functional>
#include "config.h"
#include "FractionDetector.h"

// Forward declarations
class LevelDetector;

/**
 * @brief Типы действий при переполнении
 */
enum class OverflowAction {
    SWITCH_NEXT,    // Переключить на следующий приемник
    STOP,           // Остановить процесс
    NOTIFY_ONLY     // Только уведомить (без действий)
};

/**
 * @brief Структура приемника
 */
struct Receiver {
    uint8_t id;
    String name;
    gpio_num_t controlPin;  // GPIO для управления клапаном/сервоприводом
    bool isActive;
    bool isOverflowing;
    float currentVolume;  // Текущий объем (оценочный, мл)
    float maxVolume;  // Максимальный объем (мл)
    Fraction associatedFraction;  // Связанная фракция
    
    Receiver() : id(0), name(""), controlPin(GPIO_NUM_MAX), 
                 isActive(false), isOverflowing(false),
                 currentVolume(0.0f), maxVolume(RECEIVER_MAX_VOLUME_ML),
                 associatedFraction(Fraction::UNKNOWN) {}
};

/**
 * @brief Контроллер приемников для автоматического разделения фракций
 * 
 * Управляет переключением между приемниками, интегрируется с FractionDetector
 * для автоматического переключения при смене фракций и с LevelDetector
 * для защиты от переполнения.
 */
class ReceiverController {
private:
    Receiver receivers[RECEIVER_COUNT];
    uint8_t activeReceiverId;
    LevelDetector* levelDetector;
    OverflowAction overflowAction;
    
    bool enabled;
    bool autoSwitchEnabled;  // Автоматическое переключение при смене фракций
    
    // Callbacks
    std::function<void(uint8_t receiverId)> onReceiverSwitchCallback;
    std::function<void(uint8_t receiverId)> onOverflowCallback;
    
    /**
     * @brief Деактивировать все приемники
     */
    void deactivateAll();
    
    /**
     * @brief Установить состояние GPIO пина приемника
     */
    void setReceiverPin(uint8_t receiverId, bool state);

public:
    /**
     * @brief Конструктор
     */
    ReceiverController();
    
    /**
     * @brief Инициализация контроллера
     * @param levelDet Указатель на детектор уровня
     * @return true если инициализация успешна
     */
    bool begin(LevelDetector* levelDet);
    
    /**
     * @brief Обновить состояние контроллера (вызывать периодически)
     */
    void handle();
    
    /**
     * @brief Переключить на указанный приемник
     * @param receiverId ID приемника (0-based)
     * @return true если переключение успешно
     */
    bool switchReceiver(uint8_t receiverId);
    
    /**
     * @brief Переключить приемник по фракции
     * @param fraction Фракция для переключения
     * @return true если переключение успешно
     */
    bool switchReceiverByFraction(Fraction fraction);
    
    /**
     * @brief Получить ID активного приемника
     */
    uint8_t getActiveReceiverId() const { return activeReceiverId; }
    
    /**
     * @brief Получить информацию о приемнике
     */
    const Receiver& getReceiver(uint8_t receiverId) const;
    
    /**
     * @brief Проверить переполнение активного приемника
     * @return true если обнаружено переполнение
     */
    bool checkOverflow();
    
    /**
     * @brief Установить действие при переполнении
     */
    void setOverflowAction(OverflowAction action) { overflowAction = action; }
    
    /**
     * @brief Получить текущее действие при переполнении
     */
    OverflowAction getOverflowAction() const { return overflowAction; }
    
    /**
     * @brief Включить/выключить автоматическое переключение
     */
    void setAutoSwitchEnabled(bool enable) { autoSwitchEnabled = enable; }
    
    /**
     * @brief Проверить, включено ли автоматическое переключение
     */
    bool isAutoSwitchEnabled() const { return autoSwitchEnabled; }
    
    /**
     * @brief Включить/выключить контроллер
     */
    void setEnabled(bool enable) { enabled = enable; }
    
    /**
     * @brief Проверить, включен ли контроллер
     */
    bool isEnabled() const { return enabled; }
    
    /**
     * @brief Установить callback при переключении приемника
     */
    void setOnReceiverSwitchCallback(std::function<void(uint8_t)> callback) {
        onReceiverSwitchCallback = callback;
    }
    
    /**
     * @brief Установить callback при переполнении
     */
    void setOnOverflowCallback(std::function<void(uint8_t)> callback) {
        onOverflowCallback = callback;
    }
    
    /**
     * @brief Обновить оценку объема в активном приемнике
     * @param flowRate Скорость потока (мл/сек)
     * @param deltaTime Время с последнего обновления (мс)
     */
    void updateVolume(float flowRate, unsigned long deltaTime);
    
    /**
     * @brief Сбросить объем приемника
     */
    void resetVolume(uint8_t receiverId);
    
    /**
     * @brief Настроить приемник
     */
    void configureReceiver(uint8_t receiverId, const String& name, 
                          gpio_num_t pin, float maxVolume, Fraction fraction);
};

#endif // RECEIVER_CONTROLLER_H
