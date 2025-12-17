#include "DataLogger.h"
#include "config.h"
#include <Arduino.h>

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
        record.timestamp = obj["timestamp"];
        record.alcoholPercent = obj["alcohol"];
        record.temperature = obj["temperature"];
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

void DataLogger::addMeasurement(float alcohol, float temp, bool compensated) {
    MeasurementRecord record;
    record.timestamp = millis();
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
        return MeasurementRecord{0, 0.0f, 0.0f, false};
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
        obj["alcohol"] = record.alcoholPercent;
        obj["temperature"] = record.temperature;
        obj["compensated"] = record.compensated;
    }

    String output;
    serializeJsonPretty(doc, output);
    return output;
}

void DataLogger::printFSInfo() {
    size_t totalBytes = LittleFS.totalBytes();
    size_t usedBytes = LittleFS.usedBytes();

    Serial.println("File System Info:");
    Serial.println("  Total: " + String(totalBytes) + " bytes");
    Serial.println("  Used: " + String(usedBytes) + " bytes");
    Serial.println("  Free: " + String(totalBytes - usedBytes) + " bytes");
}
