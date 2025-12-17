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
    unsigned long timestamp;
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
     */
    void addMeasurement(float alcohol, float temp, bool compensated = false);

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
     * @brief Получить информацию о файловой системе
     */
    void printFSInfo();
};

#endif // DATA_LOGGER_H
