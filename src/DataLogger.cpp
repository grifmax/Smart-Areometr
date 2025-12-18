#include <Arduino.h>
#include "SerialCompat.h"
#include "DataLogger.h"
#include "config.h"
#include <cmath>

DataLogger::DataLogger(const String &filename, int maxRecords)
    : logFile(filename), maxEntries(maxRecords) {
}

bool DataLogger::begin() {
    // Инициализация LittleFS
    if (!LittleFS.begin(true)) {  // true = форматировать если не смонтирована
        Serial.println("ERROR: Failed to mount LittleFS");
        return false;
    }

    Serial.println("LittleFS mounted successfully");
    printFSInfo();

    // Загрузить существующие записи
    loadFromFile();

    return true;
}

bool DataLogger::loadFromFile() {
    if (!LittleFS.exists(logFile)) {
        Serial.println("Log file does not exist, creating new");
        return true;
    }

    File file = LittleFS.open(logFile, "r");
    if (!file) {
        Serial.println("ERROR: Failed to open log file for reading");
        return false;
    }

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, file);
    file.close();

    if (error) {
        Serial.println("ERROR: Failed to parse log file: " + String(error.c_str()));
        return false;
    }

    records.clear();
    JsonArray array = doc["measurements"].as<JsonArray>();

    for (JsonObject obj : array) {
        MeasurementRecord record;
        record.timestamp = obj["timestamp"] | 0;
        record.unixTimestamp = obj["unix_timestamp"] | 0;  // Поддержка Unix timestamp
        record.alcoholPercent = obj["alcohol"] | 0.0f;
        record.temperature = obj["temperature"] | 0.0f;
        record.compensated = obj["compensated"] | false;
        records.push_back(record);
    }

    Serial.println("Loaded " + String(records.size()) + " records from log file");
    return true;
}

bool DataLogger::saveToFile() {
    File file = LittleFS.open(logFile, "w");
    if (!file) {
        Serial.println("ERROR: Failed to open log file for writing");
        return false;
    }

    JsonDocument doc;
    JsonArray array = doc["measurements"].to<JsonArray>();

    for (const auto &record : records) {
        JsonObject obj = array.add<JsonObject>();
        obj["timestamp"] = record.timestamp;
        obj["alcohol"] = record.alcoholPercent;
        obj["temperature"] = record.temperature;
        obj["compensated"] = record.compensated;
    }

    if (serializeJson(doc, file) == 0) {
        Serial.println("ERROR: Failed to write to log file");
        file.close();
        return false;
    }

    file.close();
    Serial.println("Saved " + String(records.size()) + " records to log file");
    return true;
}

void DataLogger::addMeasurement(float alcohol, float temp, bool compensated, unsigned long unixTime) {
    MeasurementRecord record;
    record.timestamp = millis();
    
    // Устанавливаем Unix timestamp
    if (unixTime > 0) {
        record.unixTimestamp = unixTime;
    } else {
        // Используем текущее время, если доступно
        // Для ESP32 можно использовать time(nullptr), но нужна синхронизация NTP
        // Пока используем 0 как индикатор отсутствия Unix времени
        record.unixTimestamp = 0;
    }
    
    record.alcoholPercent = alcohol;
    record.temperature = temp;
    record.compensated = compensated;

    records.push_back(record);

    // Ограничиваем количество записей
    while (records.size() > maxEntries) {
        records.erase(records.begin());
    }

    Serial.println("Measurement logged: " + String(alcohol) + "% at " + String(temp) + "°C");

    // Сохраняем в файл
    saveToFile();
}

const std::vector<MeasurementRecord>& DataLogger::getRecords() const {
    return records;
}

size_t DataLogger::getRecordCount() const {
    return records.size();
}

MeasurementRecord DataLogger::getLastRecord() const {
    if (records.empty()) {
        MeasurementRecord empty;
        empty.timestamp = 0;
        empty.unixTimestamp = 0;
        empty.alcoholPercent = 0.0f;
        empty.temperature = 0.0f;
        empty.compensated = false;
        return empty;
    }
    return records.back();
}

void DataLogger::clear() {
    records.clear();
    saveToFile();
    Serial.println("All log records cleared");
}

String DataLogger::exportToJSON() {
    JsonDocument doc;
    JsonArray array = doc["measurements"].to<JsonArray>();

    for (const auto &record : records) {
        JsonObject obj = array.add<JsonObject>();
        obj["timestamp"] = record.timestamp;
        if (record.unixTimestamp > 0) {
            obj["unix_timestamp"] = record.unixTimestamp;
        }
        obj["alcohol"] = record.alcoholPercent;
        obj["temperature"] = record.temperature;
        obj["compensated"] = record.compensated;
    }

    String output;
    serializeJsonPretty(doc, output);
    return output;
}

String DataLogger::exportToCSV(bool includeHeaders) {
    String csv = "";
    
    if (includeHeaders) {
        csv = "timestamp_ms,unix_timestamp,alcohol_percent,temperature_c,compensated\n";
    }
    
    for (const auto &record : records) {
        csv += String(record.timestamp) + ",";
        csv += String(record.unixTimestamp) + ",";
        csv += String(record.alcoholPercent, 2) + ",";
        csv += String(record.temperature, 2) + ",";
        csv += (record.compensated ? "1" : "0") + String("\n");
    }
    
    return csv;
}

std::vector<MeasurementRecord> DataLogger::getRecordsInRange(unsigned long startTime, unsigned long endTime) const {
    std::vector<MeasurementRecord> result;
    
    unsigned long actualEndTime = (endTime == 0) ? ULONG_MAX : endTime;
    
    for (const auto &record : records) {
        if (record.timestamp >= startTime && record.timestamp <= actualEndTime) {
            result.push_back(record);
        }
    }
    
    return result;
}

std::vector<MeasurementRecord> DataLogger::getRecordsLastMinutes(unsigned long minutes) const {
    if (records.empty()) {
        return std::vector<MeasurementRecord>();
    }
    
    unsigned long currentTime = millis();
    unsigned long startTime = currentTime - (minutes * 60 * 1000);
    
    // Если минуты слишком большие, используем время первой записи
    if (startTime > currentTime) {  // Переполнение unsigned long
        startTime = records[0].timestamp;
    }
    
    return getRecordsInRange(startTime, currentTime);
}

std::vector<MeasurementRecord> DataLogger::getRecordsLastHours(unsigned long hours) const {
    return getRecordsLastMinutes(hours * 60);
}

std::vector<MeasurementRecord> DataLogger::getRecordsByUnixTime(unsigned long startUnixTime, unsigned long endUnixTime) const {
    std::vector<MeasurementRecord> result;
    
    unsigned long actualEndTime = (endUnixTime == 0) ? ULONG_MAX : endUnixTime;
    
    for (const auto &record : records) {
        // Используем Unix timestamp, если доступен, иначе пропускаем запись
        if (record.unixTimestamp > 0) {
            if (record.unixTimestamp >= startUnixTime && record.unixTimestamp <= actualEndTime) {
                result.push_back(record);
            }
        }
    }
    
    return result;
}

String DataLogger::calculateStatistics(const std::vector<MeasurementRecord>& records) const {
    if (records.empty()) {
        JsonDocument doc;
        doc["count"] = 0;
        doc["error"] = "no_data";
        String output;
        serializeJson(doc, output);
        return output;
    }
    
    float minAlcohol = records[0].alcoholPercent;
    float maxAlcohol = records[0].alcoholPercent;
    float sumAlcohol = 0.0f;
    
    float minTemp = records[0].temperature;
    float maxTemp = records[0].temperature;
    float sumTemp = 0.0f;
    
    for (const auto &record : records) {
        // Статистика по крепости
        if (record.alcoholPercent < minAlcohol) minAlcohol = record.alcoholPercent;
        if (record.alcoholPercent > maxAlcohol) maxAlcohol = record.alcoholPercent;
        sumAlcohol += record.alcoholPercent;
        
        // Статистика по температуре
        if (record.temperature < minTemp) minTemp = record.temperature;
        if (record.temperature > maxTemp) maxTemp = record.temperature;
        sumTemp += record.temperature;
    }
    
    float avgAlcohol = sumAlcohol / records.size();
    float avgTemp = sumTemp / records.size();
    
    // Вычисляем стандартное отклонение для крепости
    float sumSqDev = 0.0f;
    for (const auto &record : records) {
        float dev = record.alcoholPercent - avgAlcohol;
        sumSqDev += dev * dev;
    }
    float stdDevAlcohol = sqrt(sumSqDev / records.size());
    
    JsonDocument doc;
    doc["count"] = records.size();
    doc["alcohol"]["min"] = minAlcohol;
    doc["alcohol"]["max"] = maxAlcohol;
    doc["alcohol"]["avg"] = avgAlcohol;
    doc["alcohol"]["std_dev"] = stdDevAlcohol;
    doc["temperature"]["min"] = minTemp;
    doc["temperature"]["max"] = maxTemp;
    doc["temperature"]["avg"] = avgTemp;
    
    String output;
    serializeJson(doc, output);
    return output;
}

String DataLogger::calculateStatistics() const {
    return calculateStatistics(records);
}

float DataLogger::getMinAlcohol() const {
    if (records.empty()) return 0.0f;
    float minVal = records[0].alcoholPercent;
    for (const auto &record : records) {
        if (record.alcoholPercent < minVal) {
            minVal = record.alcoholPercent;
        }
    }
    return minVal;
}

float DataLogger::getMaxAlcohol() const {
    if (records.empty()) return 0.0f;
    float maxVal = records[0].alcoholPercent;
    for (const auto &record : records) {
        if (record.alcoholPercent > maxVal) {
            maxVal = record.alcoholPercent;
        }
    }
    return maxVal;
}

float DataLogger::getAvgAlcohol() const {
    if (records.empty()) return 0.0f;
    float sum = 0.0f;
    for (const auto &record : records) {
        sum += record.alcoholPercent;
    }
    return sum / records.size();
}

float DataLogger::getAvgTemperature() const {
    if (records.empty()) return 0.0f;
    float sum = 0.0f;
    for (const auto &record : records) {
        sum += record.temperature;
    }
    return sum / records.size();
}

void DataLogger::printFSInfo() {
    size_t totalBytes = LittleFS.totalBytes();
    size_t usedBytes = LittleFS.usedBytes();

    Serial.println("File System Info:");
    Serial.println("  Total: " + String(totalBytes) + " bytes");
    Serial.println("  Used: " + String(usedBytes) + " bytes");
    Serial.println("  Free: " + String(totalBytes - usedBytes) + " bytes");
}
