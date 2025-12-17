#include "FractionDetector.h"
#include <Arduino.h>

FractionDetector::FractionDetector()
    : currentFraction(Fraction::UNKNOWN), previousFraction(Fraction::UNKNOWN),
      mode(DetectionMode::MASH_MODE),  // По умолчанию - режим из браги
      historyIndex(0), historyCount(0), currentStatsIndex(0),
      totalVolume(0), lastVolumeUpdate(0), flowRateMLPerSec(0) {

    // Дефолтные пороги (консервативные значения)
    thresholds.foreshotsVolume = 50.0f;      // Первые 50мл отбираем отдельно
    thresholds.headsThreshold = 85.0f;       // Выше 85% - головы
    thresholds.bodyMinThreshold = 78.0f;     // 78-85% - тело
    thresholds.bodyMaxThreshold = 85.0f;
    thresholds.tailsThreshold = 78.0f;       // Ниже 78% - хвосты
    thresholds.rateThreshold = 0.5f;         // Скорость падения >0.5%/мин

    // Очистка истории
    memset(alcoholHistory, 0, sizeof(alcoholHistory));
    memset(timeHistory, 0, sizeof(timeHistory));

    // Инициализация статистики
    for (int i = 0; i < 5; i++) {
        stats[i].type = static_cast<Fraction>(i);
        stats[i].startTime = 0;
        stats[i].duration = 0;
        stats[i].volumeML = 0;
        stats[i].avgAlcohol = 0;
        stats[i].minAlcohol = 100.0f;
        stats[i].maxAlcohol = 0;
    }
}

void FractionDetector::setThresholds(const FractionThresholds &t) {
    thresholds = t;
    Serial.println("FractionDetector: Thresholds updated");
}

FractionThresholds FractionDetector::getThresholds() const {
    return thresholds;
}

Fraction FractionDetector::update(float alcohol, float temperature) {
    unsigned long now = millis();

    // Добавляем в историю
    alcoholHistory[historyIndex] = alcohol;
    timeHistory[historyIndex] = now;
    historyIndex = (historyIndex + 1) % HISTORY_SIZE;
    if (historyCount < HISTORY_SIZE) historyCount++;

    // Обновляем статистику текущей фракции
    updateCurrentStats(alcohol);

    // Обновляем объем
    if (lastVolumeUpdate > 0 && flowRateMLPerSec > 0) {
        float elapsedSec = (now - lastVolumeUpdate) / 1000.0f;
        float addedVolume = flowRateMLPerSec * elapsedSec;
        totalVolume += addedVolume;

        if (currentStatsIndex < 5) {
            stats[currentStatsIndex].volumeML += addedVolume;
        }
    }
    lastVolumeUpdate = now;

    // В режиме мониторинга - не детектируем автоматически, только собираем данные
    if (mode == DetectionMode::MONITORING_MODE) {
        return currentFraction;
    }

    // Определяем фракцию (только в режиме MASH_MODE)
    Fraction newFraction = currentFraction;

    // Этап 1: Первач (foreshots) - по объему
    if (currentFraction == Fraction::UNKNOWN && totalVolume < thresholds.foreshotsVolume) {
        newFraction = Fraction::FORESHOTS;
    }
    // Этап 2: Головы (heads) - высокая крепость
    else if (alcohol >= thresholds.headsThreshold) {
        newFraction = Fraction::HEADS;
    }
    // Этап 3: Тело (body) - основная питьевая часть
    else if (alcohol >= thresholds.bodyMinThreshold && alcohol < thresholds.bodyMaxThreshold) {
        newFraction = Fraction::BODY;
    }
    // Этап 4: Хвосты (tails) - низкая крепость
    else if (alcohol < thresholds.tailsThreshold) {
        // Дополнительная проверка по скорости падения
        float rate = calculateAlcoholRate();

        // Если крепость быстро падает - точно хвосты
        if (rate < -thresholds.rateThreshold || currentFraction == Fraction::TAILS) {
            newFraction = Fraction::TAILS;
        } else if (currentFraction == Fraction::BODY) {
            // Если были в теле и крепость упала - переходим к хвостам
            newFraction = Fraction::TAILS;
        }
    }

    // Проверка на завершение (крепость очень низкая)
    if (alcohol < 5.0f && currentFraction == Fraction::TAILS) {
        newFraction = Fraction::FINISHED;
    }

    // Если фракция изменилась - вызываем callback и начинаем новую
    if (newFraction != currentFraction && newFraction != Fraction::UNKNOWN) {
        if (fractionChangeCallback) {
            fractionChangeCallback(newFraction, currentFraction);
        }
        startNewFraction(newFraction);
    }

    return currentFraction;
}

Fraction FractionDetector::getCurrentFraction() const {
    return currentFraction;
}

String FractionDetector::getFractionName(Fraction f) {
    switch (f) {
        case Fraction::UNKNOWN: return "unknown";
        case Fraction::FORESHOTS: return "foreshots";
        case Fraction::HEADS: return "heads";
        case Fraction::BODY: return "body";
        case Fraction::TAILS: return "tails";
        case Fraction::FINISHED: return "finished";
        default: return "unknown";
    }
}

String FractionDetector::getFractionColor(Fraction f) {
    switch (f) {
        case Fraction::UNKNOWN: return "#9E9E9E";
        case Fraction::FORESHOTS: return "#D32F2F";  // Темно-красный
        case Fraction::HEADS: return "#F57C00";      // Оранжевый
        case Fraction::BODY: return "#4CAF50";       // Зеленый
        case Fraction::TAILS: return "#FBC02D";      // Желтый
        case Fraction::FINISHED: return "#757575";   // Серый
        default: return "#9E9E9E";
    }
}

void FractionDetector::setFlowRate(float mlPerSec) {
    flowRateMLPerSec = mlPerSec;
    lastVolumeUpdate = millis();
}

float FractionDetector::getCurrentVolume() const {
    if (currentStatsIndex < 5) {
        return stats[currentStatsIndex].volumeML;
    }
    return 0;
}

float FractionDetector::getTotalVolume() const {
    return totalVolume;
}

void FractionDetector::reset() {
    currentFraction = Fraction::UNKNOWN;
    previousFraction = Fraction::UNKNOWN;
    historyIndex = 0;
    historyCount = 0;
    totalVolume = 0;
    lastVolumeUpdate = 0;

    // Очистка истории
    memset(alcoholHistory, 0, sizeof(alcoholHistory));
    memset(timeHistory, 0, sizeof(timeHistory));

    // Сброс статистики
    for (int i = 0; i < 5; i++) {
        stats[i].startTime = 0;
        stats[i].duration = 0;
        stats[i].volumeML = 0;
        stats[i].avgAlcohol = 0;
        stats[i].minAlcohol = 100.0f;
        stats[i].maxAlcohol = 0;
    }

    currentStatsIndex = 0;

    Serial.println("FractionDetector: Reset complete");
}

FractionStats FractionDetector::getStats(Fraction f) const {
    int index = static_cast<int>(f);
    if (index >= 0 && index < 5) {
        return stats[index];
    }
    return FractionStats();
}

String FractionDetector::getStatsJSON() const {
    JsonDocument doc;
    JsonArray fractions = doc["fractions"].to<JsonArray>();

    for (int i = 0; i < 5; i++) {
        if (stats[i].duration > 0 || stats[i].volumeML > 0) {
            JsonObject frac = fractions.add<JsonObject>();
            frac["type"] = getFractionName(stats[i].type);
            frac["duration"] = stats[i].duration / 1000;  // В секундах
            frac["volume"] = round(stats[i].volumeML * 10) / 10.0f;
            frac["avg_alcohol"] = round(stats[i].avgAlcohol * 10) / 10.0f;
            frac["min_alcohol"] = round(stats[i].minAlcohol * 10) / 10.0f;
            frac["max_alcohol"] = round(stats[i].maxAlcohol * 10) / 10.0f;
        }
    }

    doc["total_volume"] = round(totalVolume * 10) / 10.0f;
    doc["current_fraction"] = getFractionName(currentFraction);

    String json;
    serializeJson(doc, json);
    return json;
}

void FractionDetector::setFractionChangeCallback(std::function<void(Fraction, Fraction)> callback) {
    fractionChangeCallback = callback;
}

void FractionDetector::setFraction(Fraction f) {
    if (f != currentFraction) {
        Fraction oldFraction = currentFraction;
        startNewFraction(f);

        if (fractionChangeCallback) {
            fractionChangeCallback(f, oldFraction);
        }
    }
}

String FractionDetector::exportSession() const {
    return getStatsJSON();
}

bool FractionDetector::loadSettings(const String &json) {
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, json);

    if (error) {
        Serial.println("FractionDetector: Failed to parse settings JSON");
        return false;
    }

    thresholds.foreshotsVolume = doc["foreshots_volume"] | 50.0f;
    thresholds.headsThreshold = doc["heads_threshold"] | 85.0f;
    thresholds.bodyMinThreshold = doc["body_min"] | 78.0f;
    thresholds.bodyMaxThreshold = doc["body_max"] | 85.0f;
    thresholds.tailsThreshold = doc["tails_threshold"] | 78.0f;
    thresholds.rateThreshold = doc["rate_threshold"] | 0.5f;

    // Загружаем режим
    String modeStr = doc["mode"] | "mash";
    if (modeStr == "monitoring") {
        mode = DetectionMode::MONITORING_MODE;
    } else {
        mode = DetectionMode::MASH_MODE;
    }

    Serial.println("FractionDetector: Settings loaded");
    return true;
}

String FractionDetector::saveSettings() const {
    JsonDocument doc;

    doc["foreshots_volume"] = thresholds.foreshotsVolume;
    doc["heads_threshold"] = thresholds.headsThreshold;
    doc["body_min"] = thresholds.bodyMinThreshold;
    doc["body_max"] = thresholds.bodyMaxThreshold;
    doc["tails_threshold"] = thresholds.tailsThreshold;
    doc["rate_threshold"] = thresholds.rateThreshold;
    doc["mode"] = getModeName(mode);

    String json;
    serializeJson(doc, json);
    return json;
}

void FractionDetector::setMode(DetectionMode m) {
    mode = m;
    Serial.printf("FractionDetector: Mode set to %s\n", getModeName(m).c_str());
}

DetectionMode FractionDetector::getMode() const {
    return mode;
}

String FractionDetector::getModeName(DetectionMode m) {
    switch (m) {
        case DetectionMode::MASH_MODE: return "mash";
        case DetectionMode::MONITORING_MODE: return "monitoring";
        default: return "unknown";
    }
}

// === Private методы ===

float FractionDetector::calculateAlcoholRate() {
    if (historyCount < 2) return 0;

    // Берем первое и последнее значение из истории
    int oldestIndex = (historyIndex + HISTORY_SIZE - historyCount) % HISTORY_SIZE;
    int newestIndex = (historyIndex + HISTORY_SIZE - 1) % HISTORY_SIZE;

    float alcoholDiff = alcoholHistory[newestIndex] - alcoholHistory[oldestIndex];
    unsigned long timeDiff = timeHistory[newestIndex] - timeHistory[oldestIndex];

    if (timeDiff == 0) return 0;

    // Переводим в %/минуту
    float ratePerMs = alcoholDiff / (float)timeDiff;
    float ratePerMin = ratePerMs * 60000.0f;

    return ratePerMin;
}

void FractionDetector::updateCurrentStats(float alcohol) {
    if (currentStatsIndex >= 5) return;

    FractionStats &s = stats[currentStatsIndex];

    // Обновляем min/max
    if (alcohol < s.minAlcohol) s.minAlcohol = alcohol;
    if (alcohol > s.maxAlcohol) s.maxAlcohol = alcohol;

    // Обновляем среднюю (скользящее среднее)
    if (s.avgAlcohol == 0) {
        s.avgAlcohol = alcohol;
    } else {
        s.avgAlcohol = (s.avgAlcohol * 0.95f) + (alcohol * 0.05f);
    }

    // Обновляем длительность
    if (s.startTime > 0) {
        s.duration = millis() - s.startTime;
    }
}

void FractionDetector::startNewFraction(Fraction newFraction) {
    previousFraction = currentFraction;
    currentFraction = newFraction;
    currentStatsIndex = static_cast<int>(newFraction);

    if (currentStatsIndex < 5) {
        stats[currentStatsIndex].startTime = millis();
        stats[currentStatsIndex].duration = 0;
        stats[currentStatsIndex].volumeML = 0;
        stats[currentStatsIndex].avgAlcohol = 0;
        stats[currentStatsIndex].minAlcohol = 100.0f;
        stats[currentStatsIndex].maxAlcohol = 0;
    }

    Serial.printf("FractionDetector: Changed from %s to %s\n",
                  getFractionName(previousFraction).c_str(),
                  getFractionName(currentFraction).c_str());
}
