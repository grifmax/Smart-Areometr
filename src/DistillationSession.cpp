#include <Arduino.h>
#include "SerialCompat.h"
#include "DistillationSession.h"
#include <LittleFS.h>
#include <ArduinoJson.h>

DistillationSession::DistillationSession()
    : state(SessionState::IDLE), startTime(0), endTime(0),
      pauseStartTime(0), totalPauseDuration(0),
      sessionName(""), mashVolume(0), expectedYield(0),
      logInterval(5000), dataPoints(nullptr), dataPointsCount(0),
      lastLogTime(0), totalAlcoholCollected(0), totalVolumeCollected(0),
      avgAlcoholPercent(0), autoSave(true) {
    
    // Выделяем память для точек данных
    dataPoints = new DataPoint[MAX_DATA_POINTS];
    if (!dataPoints) {
        Serial.println("ERROR: Failed to allocate memory for data points");
    }
    
    sessionId = "";
    sessionFilePath = "";
}

DistillationSession::~DistillationSession() {
    if (dataPoints) {
        delete[] dataPoints;
    }
}

String DistillationSession::generateSessionId() {
    // Генерируем ID на основе времени и случайного числа
    unsigned long time = millis();
    uint32_t randomNum = random(1000, 9999);
    return "session_" + String(time) + "_" + String(randomNum);
}

bool DistillationSession::start(const String &name, float mashVol) {
    if (state == SessionState::RUNNING) {
        Serial.println("Session already running");
        return false;
    }
    
    // Очищаем предыдущие данные
    clear();
    
    sessionName = name;
    mashVolume = mashVol;
    sessionId = generateSessionId();
    startTime = millis();
    endTime = 0;
    pauseStartTime = 0;
    totalPauseDuration = 0;
    state = SessionState::RUNNING;
    lastLogTime = startTime;
    
    // Формируем путь к файлу
    sessionFilePath = "/sessions/" + sessionId + ".json";
    
    Serial.printf("Session started: %s (ID: %s)\n", sessionName.c_str(), sessionId.c_str());
    
    return true;
}

void DistillationSession::stop() {
    if (state == SessionState::IDLE || state == SessionState::FINISHED) {
        return;
    }
    
    // Если была пауза, учитываем её
    if (state == SessionState::PAUSED) {
        totalPauseDuration += millis() - pauseStartTime;
    }
    
    endTime = millis();
    state = SessionState::FINISHED;
    
    // Обновляем статистику
    updateStatistics();
    
    // Автосохранение
    if (autoSave) {
        saveToFile();
    }
    
    Serial.printf("Session stopped: %s\n", sessionName.c_str());
}

void DistillationSession::togglePause() {
    if (state == SessionState::RUNNING) {
        state = SessionState::PAUSED;
        pauseStartTime = millis();
        Serial.println("Session paused");
    } else if (state == SessionState::PAUSED) {
        state = SessionState::RUNNING;
        totalPauseDuration += millis() - pauseStartTime;
        pauseStartTime = 0;
        Serial.println("Session resumed");
    }
}

void DistillationSession::logMeasurement(float alcohol, float temperature, uint8_t stability, Fraction fraction) {
    if (state != SessionState::RUNNING) {
        return;  // Логируем только в активной сессии
    }
    
    unsigned long now = millis();
    
    // Проверяем интервал логирования
    if (now - lastLogTime < logInterval) {
        return;
    }
    
    lastLogTime = now;
    
    // Создаем точку данных
    DataPoint point;
    point.timestamp = now - startTime - totalPauseDuration;
    point.alcohol = alcohol;
    point.temperature = temperature;
    point.stability = stability;
    point.fraction = fraction;
    
    // Добавляем точку
    if (!addDataPoint(point)) {
        Serial.println("WARNING: Failed to add data point (buffer full)");
    }
    
    // Периодическое автосохранение (каждые 10 точек)
    if (autoSave && dataPointsCount % 10 == 0) {
        saveToFile();
    }
}

bool DistillationSession::addDataPoint(const DataPoint &point) {
    if (dataPointsCount >= MAX_DATA_POINTS) {
        return false;  // Буфер переполнен
    }
    
    dataPoints[dataPointsCount] = point;
    dataPointsCount++;
    
    return true;
}

void DistillationSession::updateStatistics() {
    if (dataPointsCount == 0) {
        return;
    }
    
    totalVolumeCollected = 0;
    totalAlcoholCollected = 0;
    float sumAlcohol = 0;
    
    // Подсчитываем статистику по фракциям
    float fractionVolumes[5] = {0, 0, 0, 0, 0};        // Объем каждой фракции (мл)
    float fractionAlcoholSum[5] = {0, 0, 0, 0, 0};     // Сумма крепости для усреднения
    uint16_t fractionCounts[5] = {0, 0, 0, 0, 0};      // Количество точек
    unsigned long fractionTimeSpent[5] = {0, 0, 0, 0, 0}; // Время в каждой фракции (мс)
    unsigned long previousTimestamp = 0;
    
    for (uint16_t i = 0; i < dataPointsCount; i++) {
        const DataPoint &point = dataPoints[i];
        int fracIndex = static_cast<int>(point.fraction);
        
        // Вычисляем время, проведенное в этой фракции
        if (i > 0) {
            unsigned long timeDelta = point.timestamp - previousTimestamp;
            if (fracIndex >= 0 && fracIndex < 5) {
                fractionTimeSpent[fracIndex] += timeDelta;
            }
        }
        previousTimestamp = point.timestamp;
        
        if (fracIndex >= 0 && fracIndex < 5) {
            fractionCounts[fracIndex]++;
            fractionAlcoholSum[fracIndex] += point.alcohol;
            // Предполагаем, что каждое измерение соответствует примерно 1 мл отбора
            // Это может быть настроено в зависимости от частоты измерений и реальной скорости отбора
            fractionVolumes[fracIndex] += 1.0f;
        }
        
        sumAlcohol += point.alcohol;
    }
    
    // Средняя крепость
    avgAlcoholPercent = sumAlcohol / dataPointsCount;
    
    // Общий объем (в мл, потом переведем в литры)
    // Предполагаем, что каждая точка = примерно 1 мл (можно настроить)
    totalVolumeCollected = dataPointsCount * 1.0f / 1000.0f;  // Переводим в литры
    
    // Абсолютный спирт
    totalAlcoholCollected = totalVolumeCollected * avgAlcoholPercent / 100.0f;
}

SessionState DistillationSession::getState() const {
    return state;
}

String DistillationSession::getSessionId() const {
    return sessionId;
}

unsigned long DistillationSession::getDuration() const {
    if (state == SessionState::IDLE) {
        return 0;
    }
    
    unsigned long currentTime = (state == SessionState::FINISHED) ? endTime : millis();
    unsigned long pauseTime = totalPauseDuration;
    
    if (state == SessionState::PAUSED) {
        pauseTime += millis() - pauseStartTime;
    }
    
    return (currentTime - startTime - pauseTime) / 1000;  // В секундах
}

String DistillationSession::getDurationFormatted() const {
    unsigned long seconds = getDuration();
    unsigned long hours = seconds / 3600;
    unsigned long minutes = (seconds % 3600) / 60;
    unsigned long secs = seconds % 60;
    
    char buffer[16];
    snprintf(buffer, sizeof(buffer), "%02lu:%02lu:%02lu", hours, minutes, secs);
    return String(buffer);
}

void DistillationSession::setLogInterval(uint16_t intervalMs) {
    logInterval = intervalMs;
}

void DistillationSession::setAutoSave(bool enabled) {
    autoSave = enabled;
}

uint16_t DistillationSession::getDataPointsCount() const {
    return dataPointsCount;
}

DataPoint DistillationSession::getDataPoint(uint16_t index) const {
    if (index >= dataPointsCount) {
        DataPoint empty;
        memset(&empty, 0, sizeof(DataPoint));
        return empty;
    }
    return dataPoints[index];
}

String DistillationSession::exportToJSON() const {
    JsonDocument doc;
    
    doc["session_id"] = sessionId;
    doc["session_name"] = sessionName;
    doc["state"] = (state == SessionState::RUNNING) ? "running" : 
                   (state == SessionState::PAUSED) ? "paused" :
                   (state == SessionState::FINISHED) ? "finished" : "idle";
    doc["start_time"] = startTime;
    doc["end_time"] = endTime;
    doc["duration"] = getDuration();
    doc["duration_formatted"] = getDurationFormatted();
    doc["mash_volume"] = mashVolume;
    doc["expected_yield"] = expectedYield;
    doc["total_volume_collected"] = totalVolumeCollected;
    doc["total_alcohol_collected"] = totalAlcoholCollected;
    doc["avg_alcohol_percent"] = avgAlcoholPercent;
    doc["data_points_count"] = dataPointsCount;
    doc["collection_rate"] = getCollectionRate();  // мл/мин
    doc["progress"] = getProgress();  // Процент выполнения
    
    // Статистика по фракциям
    JsonObject fractionsStats = doc["fractions"].to<JsonObject>();
    float fractionVolumes[5] = {0, 0, 0, 0, 0};
    float fractionAlcoholSum[5] = {0, 0, 0, 0, 0};
    uint16_t fractionCounts[5] = {0, 0, 0, 0, 0};
    unsigned long fractionTimeSpent[5] = {0, 0, 0, 0, 0};
    unsigned long previousTimestamp = 0;
    unsigned long totalActiveTime = 0;
    
    // Вычисляем статистику
    for (uint16_t i = 0; i < dataPointsCount; i++) {
        const DataPoint &point = dataPoints[i];
        int fracIndex = static_cast<int>(point.fraction);
        
        if (i > 0) {
            unsigned long timeDelta = point.timestamp - previousTimestamp;
            totalActiveTime += timeDelta;
            if (fracIndex >= 0 && fracIndex < 5) {
                fractionTimeSpent[fracIndex] += timeDelta;
            }
        }
        previousTimestamp = point.timestamp;
        
        if (fracIndex >= 0 && fracIndex < 5) {
            fractionCounts[fracIndex]++;
            fractionAlcoholSum[fracIndex] += point.alcohol;
            fractionVolumes[fracIndex] += 1.0f;  // Примерно 1 мл на точку
        }
    }
    
    // Добавляем статистику по каждой фракции
    Fraction fractions[] = {Fraction::FORESHOTS, Fraction::HEADS, Fraction::BODY, Fraction::TAILS, Fraction::UNKNOWN};
    for (int i = 0; i < 5; i++) {
        JsonObject fracStat = fractionsStats[FractionDetector::getFractionName(fractions[i])].to<JsonObject>();
        fracStat["volume_ml"] = fractionVolumes[i];
        fracStat["volume_l"] = fractionVolumes[i] / 1000.0f;
        fracStat["count"] = fractionCounts[i];
        
        if (fractionCounts[i] > 0) {
            fracStat["avg_alcohol"] = fractionAlcoholSum[i] / fractionCounts[i];
            float minAlcohol = 100.0f;
            float maxAlcohol = 0.0f;
            for (uint16_t j = 0; j < dataPointsCount; j++) {
                if (dataPoints[j].fraction == fractions[i]) {
                    if (dataPoints[j].alcohol < minAlcohol) minAlcohol = dataPoints[j].alcohol;
                    if (dataPoints[j].alcohol > maxAlcohol) maxAlcohol = dataPoints[j].alcohol;
                }
            }
            fracStat["min_alcohol"] = minAlcohol;
            fracStat["max_alcohol"] = maxAlcohol;
        } else {
            fracStat["avg_alcohol"] = 0.0f;
            fracStat["min_alcohol"] = 0.0f;
            fracStat["max_alcohol"] = 0.0f;
        }
        
        // Процент времени в этой фракции
        if (totalActiveTime > 0) {
            fracStat["time_percent"] = (fractionTimeSpent[i] * 100.0f) / totalActiveTime;
            fracStat["time_seconds"] = fractionTimeSpent[i] / 1000;
        } else {
            fracStat["time_percent"] = 0.0f;
            fracStat["time_seconds"] = 0;
        }
    }
    
    // Массив точек данных
    JsonArray points = doc["data_points"].to<JsonArray>();
    for (uint16_t i = 0; i < dataPointsCount; i++) {
        JsonObject point = points.add<JsonObject>();
        point["timestamp"] = dataPoints[i].timestamp;
        point["alcohol"] = dataPoints[i].alcohol;
        point["temperature"] = dataPoints[i].temperature;
        point["stability"] = dataPoints[i].stability;
        point["fraction"] = FractionDetector::getFractionName(dataPoints[i].fraction);
    }
    
    String json;
    serializeJson(doc, json);
    return json;
}

String DistillationSession::exportToCSV() const {
    String csv = "timestamp_ms,alcohol_percent,temperature_c,stability_percent,fraction\n";
    
    for (uint16_t i = 0; i < dataPointsCount; i++) {
        csv += String(dataPoints[i].timestamp) + ",";
        csv += String(dataPoints[i].alcohol, 2) + ",";
        csv += String(dataPoints[i].temperature, 2) + ",";
        csv += String(dataPoints[i].stability) + ",";
        csv += FractionDetector::getFractionName(dataPoints[i].fraction);
        csv += "\n";
    }
    
    return csv;
}

String DistillationSession::getSummary() const {
    String summary = "Session: " + sessionName + "\n";
    summary += "ID: " + sessionId + "\n";
    summary += "Duration: " + getDurationFormatted() + "\n";
    summary += "Data points: " + String(dataPointsCount) + "\n";
    summary += "Avg alcohol: " + String(avgAlcoholPercent, 1) + "%\n";
    summary += "Volume collected: " + String(totalVolumeCollected, 2) + " L\n";
    
    return summary;
}

String DistillationSession::getFractionSummary(Fraction f) const {
    // Подсчитываем статистику для конкретной фракции
    uint16_t count = 0;
    float sumAlcohol = 0;
    float minAlcohol = 100;
    float maxAlcohol = 0;
    
    for (uint16_t i = 0; i < dataPointsCount; i++) {
        if (dataPoints[i].fraction == f) {
            count++;
            sumAlcohol += dataPoints[i].alcohol;
            if (dataPoints[i].alcohol < minAlcohol) minAlcohol = dataPoints[i].alcohol;
            if (dataPoints[i].alcohol > maxAlcohol) maxAlcohol = dataPoints[i].alcohol;
        }
    }
    
    if (count == 0) {
        return "No data for " + FractionDetector::getFractionName(f);
    }
    
    float avg = sumAlcohol / count;
    String summary = FractionDetector::getFractionName(f) + ":\n";
    summary += "  Points: " + String(count) + "\n";
    summary += "  Avg alcohol: " + String(avg, 1) + "%\n";
    summary += "  Range: " + String(minAlcohol, 1) + "-" + String(maxAlcohol, 1) + "%\n";
    
    return summary;
}

void DistillationSession::clear() {
    dataPointsCount = 0;
    startTime = 0;
    endTime = 0;
    pauseStartTime = 0;
    totalPauseDuration = 0;
    totalAlcoholCollected = 0;
    totalVolumeCollected = 0;
    avgAlcoholPercent = 0;
    sessionId = "";
    sessionName = "";
    sessionFilePath = "";
}

bool DistillationSession::restoreLastSession() {
    // Ищем последнюю сессию в папке /sessions/
    if (!LittleFS.begin(false)) {
        return false;
    }
    
    // Простая реализация: ищем файл с самым поздним временем модификации
    // Для упрощения используем фиксированное имя "last_session.json"
    String lastSessionPath = "/sessions/last_session.json";
    
    if (!LittleFS.exists(lastSessionPath)) {
        return false;
    }
    
    return loadFromFile(lastSessionPath);
}

bool DistillationSession::saveToFile() {
    if (sessionFilePath.isEmpty()) {
        return false;
    }
    
    if (!LittleFS.begin(false)) {
        Serial.println("ERROR: Failed to mount LittleFS for saving session");
        return false;
    }
    
    // Создаем директорию если не существует
    if (!LittleFS.exists("/sessions")) {
        // LittleFS не поддерживает директории напрямую, используем префикс в имени файла
        // Но для совместимости создадим файл с путем
    }
    
    File file = LittleFS.open(sessionFilePath, "w");
    if (!file) {
        Serial.println("ERROR: Failed to open session file for writing");
        return false;
    }
    
    String json = exportToJSON();
    file.print(json);
    file.close();
    
    // Также сохраняем как последнюю сессию
    File lastFile = LittleFS.open("/sessions/last_session.json", "w");
    if (lastFile) {
        lastFile.print(json);
        lastFile.close();
    }
    
    Serial.printf("Session saved to %s\n", sessionFilePath.c_str());
    return true;
}

bool DistillationSession::loadFromFile(const String &path) {
    if (!LittleFS.begin(false)) {
        return false;
    }
    
    if (!LittleFS.exists(path)) {
        return false;
    }
    
    File file = LittleFS.open(path, "r");
    if (!file) {
        return false;
    }
    
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, file);
    file.close();
    
    if (error) {
        Serial.println("ERROR: Failed to parse session file");
        return false;
    }
    
    // Загружаем данные
    sessionId = doc["session_id"] | "";
    sessionName = doc["session_name"] | "";
    startTime = doc["start_time"] | 0;
    endTime = doc["end_time"] | 0;
    mashVolume = doc["mash_volume"] | 0.0f;
    expectedYield = doc["expected_yield"] | 0.0f;
    totalVolumeCollected = doc["total_volume_collected"] | 0.0f;
    totalAlcoholCollected = doc["total_alcohol_collected"] | 0.0f;
    avgAlcoholPercent = doc["avg_alcohol_percent"] | 0.0f;
    
    String stateStr = doc["state"] | "idle";
    if (stateStr == "running") {
        state = SessionState::RUNNING;
    } else if (stateStr == "paused") {
        state = SessionState::PAUSED;
    } else if (stateStr == "finished") {
        state = SessionState::FINISHED;
    } else {
        state = SessionState::IDLE;
    }
    
    // Загружаем точки данных
    JsonArray points = doc["data_points"].as<JsonArray>();
    dataPointsCount = 0;
    
    for (JsonObject point : points) {
        if (dataPointsCount >= MAX_DATA_POINTS) {
            break;
        }
        
        dataPoints[dataPointsCount].timestamp = point["timestamp"] | 0;
        dataPoints[dataPointsCount].alcohol = point["alcohol"] | 0.0f;
        dataPoints[dataPointsCount].temperature = point["temperature"] | 0.0f;
        dataPoints[dataPointsCount].stability = point["stability"] | 0;
        
        String fracStr = point["fraction"] | "unknown";
        // Преобразуем строку в Fraction
        if (fracStr == "foreshots") {
            dataPoints[dataPointsCount].fraction = Fraction::FORESHOTS;
        } else if (fracStr == "heads") {
            dataPoints[dataPointsCount].fraction = Fraction::HEADS;
        } else if (fracStr == "body") {
            dataPoints[dataPointsCount].fraction = Fraction::BODY;
        } else if (fracStr == "tails") {
            dataPoints[dataPointsCount].fraction = Fraction::TAILS;
        } else {
            dataPoints[dataPointsCount].fraction = Fraction::UNKNOWN;
        }
        
        dataPointsCount++;
    }
    
    sessionFilePath = path;
    
    Serial.printf("Session loaded from %s (%d points)\n", path.c_str(), dataPointsCount);
    return true;
}

String DistillationSession::getSessionsList() {
    JsonDocument doc;
    JsonArray sessions = doc["sessions"].to<JsonArray>();
    
    if (!LittleFS.begin(false)) {
        return "{\"sessions\":[]}";
    }
    
    // Простая реализация: возвращаем список известных сессий
    // В реальной реализации нужно сканировать директорию /sessions/
    // Но LittleFS не поддерживает listDir напрямую для всех платформ
    
    String json;
    serializeJson(doc, json);
    return json;
}

bool DistillationSession::deleteSession(const String &sessionId) {
    if (!LittleFS.begin(false)) {
        return false;
    }
    
    String path = "/sessions/" + sessionId + ".json";
    if (LittleFS.exists(path)) {
        return LittleFS.remove(path);
    }
    
    return false;
}

void DistillationSession::setExpectedYield(float liters) {
    expectedYield = liters;
}

float DistillationSession::getProgress() const {
    if (expectedYield <= 0) {
        return 0;
    }
    
    return (totalVolumeCollected / expectedYield) * 100.0f;
}

float DistillationSession::getCollectionRate() const {
    unsigned long duration = getDuration();
    if (duration == 0) {
        return 0;
    }
    
    // Скорость в мл/мин
    return (totalVolumeCollected * 1000.0f) / (duration / 60.0f);
}

String DistillationSession::getEstimatedTimeToComplete() const {
    if (expectedYield <= 0 || totalVolumeCollected >= expectedYield) {
        return "N/A";
    }
    
    float rate = getCollectionRate();  // мл/мин
    if (rate <= 0) {
        return "N/A";
    }
    
    float remaining = (expectedYield - totalVolumeCollected) * 1000.0f;  // мл
    unsigned long minutes = remaining / rate;
    
    unsigned long hours = minutes / 60;
    unsigned long mins = minutes % 60;
    
    char buffer[16];
    snprintf(buffer, sizeof(buffer), "%02lu:%02lu", hours, mins);
    return String(buffer);
}
