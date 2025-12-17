#ifndef FRACTION_DETECTOR_H
#define FRACTION_DETECTOR_H

#include <Arduino.h>
#include <ArduinoJson.h>

/**
 * @brief Типы фракций при перегонке
 */
enum class Fraction {
    UNKNOWN,    // Неизвестно / не начато
    FORESHOTS,  // Первач (метанол и т.д.) - обычно первые 30-50 мл
    HEADS,      // Головы (ацетон, эфиры) - 85%+
    BODY,       // Тело (питьевая часть) - 78-85%
    TAILS,      // Хвосты (сивушные масла) - <78%
    FINISHED    // Завершено
};

/**
 * @brief Настройки порогов фракций
 */
struct FractionThresholds {
    float foreshotsVolume;    // Объем первача (мл)
    float headsThreshold;     // Порог голов (%, >этого - головы)
    float bodyMinThreshold;   // Минимум тела (%)
    float bodyMaxThreshold;   // Максимум тела (%)
    float tailsThreshold;     // Порог хвостов (%, <этого - хвосты)
    float rateThreshold;      // Порог скорости изменения (%/мин)
};

/**
 * @brief Статистика по фракции
 */
struct FractionStats {
    Fraction type;
    unsigned long startTime;      // Время начала фракции (ms)
    unsigned long duration;       // Длительность (ms)
    float volumeML;              // Собранный объем (мл)
    float avgAlcohol;            // Средняя крепость (%)
    float minAlcohol;            // Минимальная крепость (%)
    float maxAlcohol;            // Максимальная крепость (%)
};

/**
 * @brief Детектор фракций при перегонке
 *
 * Автоматически определяет текущую фракцию на основе:
 * - Абсолютного значения крепости
 * - Скорости изменения крепости
 * - Собранного объема
 * - Времени перегонки
 *
 * Генерирует события при смене фракций для автоматического
 * переключения приемников.
 *
 * Приоритет: P2 (High) - будет реализовано в v2.1.0
 */
class FractionDetector {
private:
    FractionThresholds thresholds;
    Fraction currentFraction;
    Fraction previousFraction;

    // История измерений для расчета скорости изменения
    static const uint8_t HISTORY_SIZE = 10;
    float alcoholHistory[HISTORY_SIZE];
    unsigned long timeHistory[HISTORY_SIZE];
    uint8_t historyIndex;
    uint8_t historyCount;

    // Статистика по фракциям
    FractionStats stats[5];  // Для каждого типа фракции
    uint8_t currentStatsIndex;

    // Счетчик объема
    float totalVolume;
    unsigned long lastVolumeUpdate;
    float flowRateMLPerSec;

    // Callback при смене фракции
    std::function<void(Fraction, Fraction)> fractionChangeCallback;

    /**
     * @brief Рассчитать скорость изменения крепости
     * @return Скорость в %/минуту
     */
    float calculateAlcoholRate();

    /**
     * @brief Обновить статистику текущей фракции
     */
    void updateCurrentStats(float alcohol);

    /**
     * @brief Начать новую фракцию
     */
    void startNewFraction(Fraction newFraction);

public:
    /**
     * @brief Конструктор с дефолтными порогами
     */
    FractionDetector();

    /**
     * @brief Установить пороги фракций
     */
    void setThresholds(const FractionThresholds &t);

    /**
     * @brief Получить текущие пороги
     */
    FractionThresholds getThresholds() const;

    /**
     * @brief Обновить измерение и определить фракцию
     * @param alcohol Текущая крепость (%)
     * @param temperature Температура (°C)
     * @return Текущая фракция
     */
    Fraction update(float alcohol, float temperature);

    /**
     * @brief Получить текущую фракцию
     */
    Fraction getCurrentFraction() const;

    /**
     * @brief Получить название фракции
     */
    static String getFractionName(Fraction f);

    /**
     * @brief Получить цвет фракции для UI
     */
    static String getFractionColor(Fraction f);

    /**
     * @brief Установить скорость потока для расчета объема
     * @param mlPerSec Скорость в мл/сек
     */
    void setFlowRate(float mlPerSec);

    /**
     * @brief Получить собранный объем текущей фракции
     */
    float getCurrentVolume() const;

    /**
     * @brief Получить общий объем
     */
    float getTotalVolume() const;

    /**
     * @brief Сбросить счетчики (новая перегонка)
     */
    void reset();

    /**
     * @brief Получить статистику по фракции
     */
    FractionStats getStats(Fraction f) const;

    /**
     * @brief Получить всю статистику в JSON
     */
    String getStatsJSON() const;

    /**
     * @brief Установить callback при смене фракции
     * @param callback Функция (newFraction, oldFraction)
     */
    void setFractionChangeCallback(std::function<void(Fraction, Fraction)> callback);

    /**
     * @brief Принудительно установить фракцию (ручной режим)
     */
    void setFraction(Fraction f);

    /**
     * @brief Экспорт сессии в JSON
     */
    String exportSession() const;

    /**
     * @brief Импорт настроек из JSON
     */
    bool loadSettings(const String &json);

    /**
     * @brief Сохранить настройки в JSON
     */
    String saveSettings() const;
};

#endif // FRACTION_DETECTOR_H
