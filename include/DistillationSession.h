#ifndef DISTILLATION_SESSION_H
#define DISTILLATION_SESSION_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include "FractionDetector.h"

/**
 * @brief Точка данных в сессии
 */
struct DataPoint {
    unsigned long timestamp;    // Время от начала (ms)
    float alcohol;             // Крепость (%)
    float temperature;         // Температура (°C)
    uint8_t stability;         // Стабильность (%)
    Fraction fraction;         // Текущая фракция
};

/**
 * @brief Состояние сессии перегонки
 */
enum class SessionState {
    IDLE,       // Не начата
    RUNNING,    // В процессе
    PAUSED,     // На паузе
    FINISHED    // Завершена
};

/**
 * @brief Менеджер сессии перегонки
 *
 * Управляет процессом перегонки:
 * - Запись всех измерений с временными метками
 * - Статистика по фракциям
 * - Экспорт данных в JSON/CSV
 * - Автоматическое сохранение в LittleFS
 * - Восстановление после перезагрузки
 *
 * Приоритет: P3 (High) - будет реализовано в v2.1.0
 */
class DistillationSession {
private:
    SessionState state;
    String sessionId;
    unsigned long startTime;
    unsigned long endTime;
    unsigned long pauseStartTime;
    unsigned long totalPauseDuration;

    // Настройки
    String sessionName;
    float mashVolume;          // Объем браги (л)
    float expectedYield;       // Ожидаемый выход (л)
    uint16_t logInterval;      // Интервал записи (ms)

    // Данные
    static const uint16_t MAX_DATA_POINTS = 1000;
    DataPoint* dataPoints;
    uint16_t dataPointsCount;
    unsigned long lastLogTime;

    // Статистика
    float totalAlcoholCollected;  // Абсолютный спирт (л)
    float totalVolumeCollected;   // Общий объем (л)
    float avgAlcoholPercent;      // Средняя крепость

    // Файл для сохранения
    String sessionFilePath;
    bool autoSave;

    /**
     * @brief Генерация уникального ID сессии
     */
    String generateSessionId();

    /**
     * @brief Добавить точку данных
     */
    bool addDataPoint(const DataPoint &point);

    /**
     * @brief Обновить статистику
     */
    void updateStatistics();

    /**
     * @brief Сохранить в файл
     */
    bool saveToFile();

    /**
     * @brief Загрузить из файла
     */
    bool loadFromFile(const String &path);

public:
    /**
     * @brief Конструктор
     */
    DistillationSession();

    /**
     * @brief Деструктор
     */
    ~DistillationSession();

    /**
     * @brief Начать новую сессию
     * @param name Название сессии
     * @param mashVol Объем браги (л)
     * @return true если успешно
     */
    bool start(const String &name, float mashVol = 0);

    /**
     * @brief Остановить сессию
     */
    void stop();

    /**
     * @brief Пауза/продолжить
     */
    void togglePause();

    /**
     * @brief Записать измерение
     */
    void logMeasurement(float alcohol, float temperature, uint8_t stability, Fraction fraction);

    /**
     * @brief Получить состояние
     */
    SessionState getState() const;

    /**
     * @brief Получить ID сессии
     */
    String getSessionId() const;

    /**
     * @brief Получить длительность (секунды)
     */
    unsigned long getDuration() const;

    /**
     * @brief Получить длительность в формате строки (HH:MM:SS)
     */
    String getDurationFormatted() const;

    /**
     * @brief Установить интервал логирования
     */
    void setLogInterval(uint16_t intervalMs);

    /**
     * @brief Включить/выключить автосохранение
     */
    void setAutoSave(bool enabled);

    /**
     * @brief Получить количество точек данных
     */
    uint16_t getDataPointsCount() const;

    /**
     * @brief Получить точку данных по индексу
     */
    DataPoint getDataPoint(uint16_t index) const;

    /**
     * @brief Экспорт в JSON
     */
    String exportToJSON() const;

    /**
     * @brief Экспорт в CSV
     */
    String exportToCSV() const;

    /**
     * @brief Получить краткую статистику
     */
    String getSummary() const;

    /**
     * @brief Получить статистику по фракции
     */
    String getFractionSummary(Fraction f) const;

    /**
     * @brief Очистить данные сессии
     */
    void clear();

    /**
     * @brief Восстановить последнюю сессию
     */
    bool restoreLastSession();

    /**
     * @brief Получить список сохраненных сессий
     */
    static String getSessionsList();

    /**
     * @brief Удалить сессию
     */
    static bool deleteSession(const String &sessionId);

    /**
     * @brief Установить ожидаемый выход
     */
    void setExpectedYield(float liters);

    /**
     * @brief Получить процент выполнения
     */
    float getProgress() const;

    /**
     * @brief Получить скорость отбора (мл/мин)
     */
    float getCollectionRate() const;

    /**
     * @brief Прогноз времени завершения
     */
    String getEstimatedTimeToComplete() const;
};

#endif // DISTILLATION_SESSION_H
