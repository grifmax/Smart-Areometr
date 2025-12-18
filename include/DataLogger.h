#ifndef DATA_LOGGER_H
#define DATA_LOGGER_H

#include <Arduino.h>
#include <FS.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <vector>
#include "config.h"

/**
 * @brief Структура записи измерения
 */
struct MeasurementRecord {
    unsigned long timestamp;    // Время от начала (ms) - для обратной совместимости
    unsigned long unixTimestamp; // Unix timestamp в секундах (0 если не установлен)
    float alcoholPercent;
    float temperature;
    bool compensated;
};

/**
 * @brief Класс для логирования измерений
 *
 * Сохраняет историю измерений в файловую систему LittleFS
 * и предоставляет доступ к записям
 */
class DataLogger {
private:
    String logFile;
    int maxEntries;
    std::vector<MeasurementRecord> records;

    /**
     * @brief Загрузить записи из файла
     */
    bool loadFromFile();

    /**
     * @brief Сохранить записи в файл
     */
    bool saveToFile();

public:
    /**
     * @brief Конструктор
     * @param filename Имя файла лога
     * @param maxRecords Максимальное количество записей
     */
    DataLogger(const String &filename = LOG_FILE, int maxRecords = MAX_LOG_ENTRIES);

    /**
     * @brief Инициализация логгера
     * @return true если успешно
     */
    bool begin();

    /**
     * @brief Добавить запись измерения
     * @param alcohol Процент алкоголя
     * @param temp Температура
     * @param compensated Применена ли компенсация
     * @param unixTime Unix timestamp в секундах (0 = использовать текущее время)
     */
    void addMeasurement(float alcohol, float temp, bool compensated = false, unsigned long unixTime = 0);

    /**
     * @brief Получить все записи
     */
    const std::vector<MeasurementRecord>& getRecords() const;

    /**
     * @brief Получить количество записей
     */
    size_t getRecordCount() const;

    /**
     * @brief Получить последнюю запись
     */
    MeasurementRecord getLastRecord() const;

    /**
     * @brief Очистить все записи
     */
    void clear();

    /**
     * @brief Экспортировать записи в JSON
     */
    String exportToJSON();

    /**
     * @brief Экспортировать записи в CSV
     * @param includeHeaders Включить заголовки CSV
     */
    String exportToCSV(bool includeHeaders = true);

    /**
     * @brief Получить записи за указанный период (в миллисекундах от старта)
     * @param startTime Начальное время (millis())
     * @param endTime Конечное время (millis()), 0 = до конца
     */
    std::vector<MeasurementRecord> getRecordsInRange(unsigned long startTime, unsigned long endTime = 0) const;

    /**
     * @brief Получить записи за последние N минут (относительно текущего millis())
     * @param minutes Количество минут
     */
    std::vector<MeasurementRecord> getRecordsLastMinutes(unsigned long minutes) const;
    
    /**
     * @brief Получить записи за последние N часов
     * @param hours Количество часов
     */
    std::vector<MeasurementRecord> getRecordsLastHours(unsigned long hours) const;
    
    /**
     * @brief Получить записи по Unix timestamp диапазону
     * @param startUnixTime Начальное время (Unix timestamp в секундах)
     * @param endUnixTime Конечное время (Unix timestamp в секундах), 0 = до конца
     */
    std::vector<MeasurementRecord> getRecordsByUnixTime(unsigned long startUnixTime, unsigned long endUnixTime = 0) const;

    /**
     * @brief Вычислить статистику для записей
     * @param records Вектор записей для анализа
     * @return JSON строка со статистикой
     */
    String calculateStatistics(const std::vector<MeasurementRecord>& records) const;

    /**
     * @brief Вычислить статистику для всех записей
     */
    String calculateStatistics() const;
    
    /**
     * @brief Получить минимальное значение крепости
     */
    float getMinAlcohol() const;
    
    /**
     * @brief Получить максимальное значение крепости
     */
    float getMaxAlcohol() const;
    
    /**
     * @brief Получить среднее значение крепости
     */
    float getAvgAlcohol() const;
    
    /**
     * @brief Получить среднее значение температуры
     */
    float getAvgTemperature() const;

    /**
     * @brief Получить информацию о файловой системе
     */
    void printFSInfo();
};

#endif // DATA_LOGGER_H
