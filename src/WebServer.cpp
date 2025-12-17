#include "WebServer.h"
#include "config.h"
#include "SerialCompat.h"
#include <LittleFS.h>

WebServerManager::WebServerManager(uint16_t port)
    : apMode(false), deviceIP("") {
    server = new AsyncWebServer(port);
}

WebServerManager::~WebServerManager() {
    stop();
    delete server;
}

bool WebServerManager::beginClient(const String &s, const String &p) {
    ssid = s;
    password = p;
    apMode = false;

    Serial.println("Connecting to Wi-Fi: " + ssid);

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid.c_str(), password.c_str());

    // Ожидание подключения (максимум 20 секунд)
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 40) {
        delay(500);
        Serial.print(".");
        attempts++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        deviceIP = WiFi.localIP().toString();
        Serial.println("\nConnected! IP: " + deviceIP);

        setupRoutes();
        setupOTA();
        server->begin();

        return true;
    } else {
        Serial.println("\nFailed to connect to Wi-Fi");
        return false;
    }
}

bool WebServerManager::beginAP(const String &s, const String &p) {
    ssid = s;
    password = p;
    apMode = true;

    Serial.println("Creating Access Point: " + ssid);

    WiFi.mode(WIFI_AP);
    bool success = WiFi.softAP(ssid.c_str(), password.c_str());

    if (success) {
        deviceIP = WiFi.softAPIP().toString();
        Serial.println("AP Created! IP: " + deviceIP);

        setupRoutes();
        setupOTA();
        server->begin();

        return true;
    } else {
        Serial.println("Failed to create AP");
        return false;
    }
}

void WebServerManager::setupRoutes() {
    // Статические файлы из LittleFS
    // Главная страница
    server->on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (LittleFS.exists("/index.html")) {
            request->send(LittleFS, "/index.html", "text/html");
        } else {
            request->send(404, "text/plain", "index.html not found");
        }
    });

    // HTML страницы
    server->on("/calibration.html", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(LittleFS, "/calibration.html", "text/html");
    });

    server->on("/settings.html", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(LittleFS, "/settings.html", "text/html");
    });

    server->on("/logs.html", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(LittleFS, "/logs.html", "text/html");
    });

    server->on("/fractions.html", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(LittleFS, "/fractions.html", "text/html");
    });

    server->on("/mqtt.html", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(LittleFS, "/mqtt.html", "text/html");
    });

    // CSS файлы
    server->on("/css/style.css", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(LittleFS, "/css/style.css", "text/css");
    });

    server->on("/css/calibration.css", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(LittleFS, "/css/calibration.css", "text/css");
    });

    server->on("/css/logs.css", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(LittleFS, "/css/logs.css", "text/css");
    });

    // JavaScript файлы
    server->on("/js/app.js", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(LittleFS, "/js/app.js", "application/javascript");
    });

    server->on("/js/calibration.js", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(LittleFS, "/js/calibration.js", "application/javascript");
    });

    server->on("/js/settings.js", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(LittleFS, "/js/settings.js", "application/javascript");
    });

    server->on("/js/logs.js", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(LittleFS, "/js/logs.js", "application/javascript");
    });

    server->on("/js/fractions.js", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(LittleFS, "/js/fractions.js", "application/javascript");
    });

    server->on("/js/mqtt.js", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(LittleFS, "/js/mqtt.js", "application/javascript");
    });

    // API: Получить данные измерений
    server->on("/api/measurement", HTTP_GET, [this](AsyncWebServerRequest *request) {
        request->send(200, "application/json", generateMeasurementJSON());
    });

    // API: Получить статус
    server->on("/api/status", HTTP_GET, [this](AsyncWebServerRequest *request) {
        JsonDocument doc;
        doc["firmware"] = FIRMWARE_VERSION;
        doc["wifi_mode"] = apMode ? "AP" : "Client";
        doc["ssid"] = ssid;
        doc["ip"] = deviceIP;
        doc["calibrated"] = calibratedCallback ? calibratedCallback() : false;

        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
    });

    // API: Получить логи
    server->on("/api/logs", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (LittleFS.exists(LOG_FILE)) {
            request->send(LittleFS, LOG_FILE, "application/json");
        } else {
            request->send(200, "application/json", "{\"measurements\":[]}");
        }
    });

    // API: Очистить логи
    server->on("/api/logs", HTTP_DELETE, [](AsyncWebServerRequest *request) {
        if (LittleFS.exists(LOG_FILE)) {
            LittleFS.remove(LOG_FILE);
        }
        request->send(200, "application/json", "{\"status\":\"success\"}");
    });

    // API: Калибровка воды
    server->on("/api/calibrate/water", HTTP_POST, [](AsyncWebServerRequest *request) {
        // Этот эндпоинт будет вызывать калибровку через callback
        request->send(200, "application/json", "{\"status\":\"calibration_started\",\"step\":\"water\"}");
    });

    // API: Калибровка спирта
    server->on("/api/calibrate/alcohol", HTTP_POST, [](AsyncWebServerRequest *request) {
        request->send(200, "application/json", "{\"status\":\"calibration_started\",\"step\":\"alcohol\"}");
    });

    // API: Получить данные калибровки
    server->on("/api/calibration", HTTP_GET, [this](AsyncWebServerRequest *request) {
        if (getCalibrationDataCallback) {
            String data = getCalibrationDataCallback();
            request->send(200, "application/json", data);
        } else {
            request->send(404, "application/json", "{\"error\":\"not_calibrated\",\"points\":[]}");
        }
    });

    // API: Добавить калибровочную точку
    server->on("/api/calibration/point", HTTP_POST,
        [](AsyncWebServerRequest *request) {},
        nullptr,
        [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            // Парсим JSON из тела запроса
            JsonDocument doc;
            DeserializationError error = deserializeJson(doc, (const char*)data, len);

            if (error) {
                request->send(400, "application/json", "{\"error\":\"invalid_json\"}");
                return;
            }

            float alcoholPercent = doc["alcohol_percent"] | 0.0f;
            float temperature = doc["temperature"] | 20.0f;

            if (addCalibrationPointCallback) {
                bool success = addCalibrationPointCallback(alcoholPercent, temperature);
                if (success) {
                    // Получаем текущее сырое значение для ответа
                    uint16_t rawValue = rawValueCallback ? rawValueCallback() : 0;
                    String response = "{\"status\":\"success\",\"alcohol_percent\":" + String(alcoholPercent) +
                                     ",\"raw_value\":" + String(rawValue) + "}";
                    request->send(200, "application/json", response);
                } else {
                    request->send(500, "application/json", "{\"error\":\"failed_to_add_point\",\"message\":\"Maximum points reached or sensor error\"}");
                }
            } else {
                request->send(500, "application/json", "{\"error\":\"callback_not_set\"}");
            }
        }
    );

    // API: Удалить калибровочную точку
    server->on("/api/calibration/point/*", HTTP_DELETE, [this](AsyncWebServerRequest *request) {
        // Извлекаем индекс из URL
        String url = request->url();
        int lastSlash = url.lastIndexOf('/');
        if (lastSlash == -1) {
            request->send(400, "application/json", "{\"error\":\"invalid_url\"}");
            return;
        }

        uint8_t index = url.substring(lastSlash + 1).toInt();

        if (deleteCalibrationPointCallback) {
            bool success = deleteCalibrationPointCallback(index);
            if (success) {
                request->send(200, "application/json", "{\"status\":\"success\"}");
            } else {
                request->send(400, "application/json", "{\"error\":\"invalid_index\"}");
            }
        } else {
            request->send(500, "application/json", "{\"error\":\"callback_not_set\"}");
        }
    });

    // API: Очистить все точки калибровки
    server->on("/api/calibration", HTTP_DELETE, [this](AsyncWebServerRequest *request) {
        if (clearCalibrationCallback) {
            clearCalibrationCallback();
            request->send(200, "application/json", "{\"status\":\"success\"}");
        } else {
            request->send(500, "application/json", "{\"error\":\"callback_not_set\"}");
        }
    });

    // === Fraction Detector API ===

    // API: Получить статус фракций
    server->on("/api/fractions/status", HTTP_GET, [this](AsyncWebServerRequest *request) {
        if (getFractionStatusCallback) {
            String json = getFractionStatusCallback();
            request->send(200, "application/json", json);
        } else {
            request->send(500, "application/json", "{\"error\":\"callback_not_set\"}");
        }
    });

    // API: Получить статистику фракций
    server->on("/api/fractions/stats", HTTP_GET, [this](AsyncWebServerRequest *request) {
        if (getFractionStatsCallback) {
            String json = getFractionStatsCallback();
            request->send(200, "application/json", json);
        } else {
            request->send(500, "application/json", "{\"error\":\"callback_not_set\"}");
        }
    });

    // API: Получить пороги фракций
    server->on("/api/fractions/thresholds", HTTP_GET, [this](AsyncWebServerRequest *request) {
        if (getFractionThresholdsCallback) {
            String json = getFractionThresholdsCallback();
            request->send(200, "application/json", json);
        } else {
            request->send(500, "application/json", "{\"error\":\"callback_not_set\"}");
        }
    });

    // API: Установить пороги фракций
    server->on("/api/fractions/thresholds", HTTP_POST,
        [](AsyncWebServerRequest *request) {
            // Заглушка для обработки без тела
        },
        nullptr,
        [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            if (setFractionThresholdsCallback) {
                String body = String((char*)data).substring(0, len);
                bool success = setFractionThresholdsCallback(body);
                if (success) {
                    request->send(200, "application/json", "{\"status\":\"success\"}");
                } else {
                    request->send(400, "application/json", "{\"error\":\"invalid_data\"}");
                }
            } else {
                request->send(500, "application/json", "{\"error\":\"callback_not_set\"}");
            }
        }
    );

    // API: Сбросить сессию фракций
    server->on("/api/fractions/reset", HTTP_POST, [this](AsyncWebServerRequest *request) {
        if (resetFractionSessionCallback) {
            resetFractionSessionCallback();
            request->send(200, "application/json", "{\"status\":\"success\"}");
        } else {
            request->send(500, "application/json", "{\"error\":\"callback_not_set\"}");
        }
    });

    // API: Получить режим работы
    server->on("/api/fractions/mode", HTTP_GET, [this](AsyncWebServerRequest *request) {
        if (getFractionModeCallback) {
            String json = getFractionModeCallback();
            request->send(200, "application/json", json);
        } else {
            request->send(500, "application/json", "{\"error\":\"callback_not_set\"}");
        }
    });

    // API: Установить режим работы
    server->on("/api/fractions/mode", HTTP_POST,
        [](AsyncWebServerRequest *request) {
            // Заглушка для обработки без тела
        },
        nullptr,
        [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            if (setFractionModeCallback) {
                String body = String((char*)data).substring(0, len);
                bool success = setFractionModeCallback(body);
                if (success) {
                    request->send(200, "application/json", "{\"status\":\"success\"}");
                } else {
                    request->send(400, "application/json", "{\"error\":\"invalid_data\"}");
                }
            } else {
                request->send(500, "application/json", "{\"error\":\"callback_not_set\"}");
            }
        }
    );

    // 404
    server->onNotFound([](AsyncWebServerRequest *request) {
        request->send(404, "text/plain", "Not Found");
    });

    Serial.println("Web server routes configured");
}

void WebServerManager::setupOTA() {
    ArduinoOTA.setHostname(OTA_HOSTNAME);
    ArduinoOTA.setPassword(OTA_PASSWORD);

    ArduinoOTA.onStart([]() {
        String type = (ArduinoOTA.getCommand() == U_FLASH) ? "sketch" : "filesystem";
        Serial.println("OTA Update Start: " + type);
    });

    ArduinoOTA.onEnd([]() {
        Serial.println("\nOTA Update Complete");
    });

    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    });

    ArduinoOTA.onError([](ota_error_t error) {
        Serial.printf("Error[%u]: ", error);
        if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
        else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
        else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
        else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
        else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });

    ArduinoOTA.begin();
    Serial.println("OTA configured");
}

// Функция generateHomePage() удалена - теперь используются статические файлы из LittleFS

String WebServerManager::generateMeasurementJSON() {
    JsonDocument doc;

    doc["alcohol"] = alcoholCallback ? alcoholCallback() : 0.0f;
    doc["temperature"] = temperatureCallback ? temperatureCallback() : 0.0f;
    doc["calibrated"] = calibratedCallback ? calibratedCallback() : false;
    doc["raw_value"] = rawValueCallback ? rawValueCallback() : 0;
    doc["stability"] = stabilityCallback ? stabilityCallback() : 0;
    doc["timestamp"] = millis();

    String response;
    serializeJson(doc, response);
    return response;
}

String WebServerManager::getIP() const {
    return deviceIP;
}

String WebServerManager::getSSID() const {
    return ssid;
}

bool WebServerManager::isConnected() const {
    return apMode ? true : (WiFi.status() == WL_CONNECTED);
}

bool WebServerManager::isAPMode() const {
    return apMode;
}

void WebServerManager::setAlcoholCallback(std::function<float()> callback) {
    alcoholCallback = callback;
}

void WebServerManager::setTemperatureCallback(std::function<float()> callback) {
    temperatureCallback = callback;
}

void WebServerManager::setCalibratedCallback(std::function<bool()> callback) {
    calibratedCallback = callback;
}

void WebServerManager::setRawValueCallback(std::function<uint16_t()> callback) {
    rawValueCallback = callback;
}

void WebServerManager::setStabilityCallback(std::function<uint8_t()> callback) {
    stabilityCallback = callback;
}

void WebServerManager::setAddCalibrationPointCallback(std::function<bool(float, float)> callback) {
    addCalibrationPointCallback = callback;
}

void WebServerManager::setDeleteCalibrationPointCallback(std::function<bool(uint8_t)> callback) {
    deleteCalibrationPointCallback = callback;
}

void WebServerManager::setClearCalibrationCallback(std::function<void()> callback) {
    clearCalibrationCallback = callback;
}

void WebServerManager::setGetCalibrationDataCallback(std::function<String()> callback) {
    getCalibrationDataCallback = callback;
}

void WebServerManager::setGetFractionStatusCallback(std::function<String()> callback) {
    getFractionStatusCallback = callback;
}

void WebServerManager::setGetFractionStatsCallback(std::function<String()> callback) {
    getFractionStatsCallback = callback;
}

void WebServerManager::setGetFractionThresholdsCallback(std::function<String()> callback) {
    getFractionThresholdsCallback = callback;
}

void WebServerManager::setSetFractionThresholdsCallback(std::function<bool(const String&)> callback) {
    setFractionThresholdsCallback = callback;
}

void WebServerManager::setResetFractionSessionCallback(std::function<void()> callback) {
    resetFractionSessionCallback = callback;
}

void WebServerManager::setGetFractionModeCallback(std::function<String()> callback) {
    getFractionModeCallback = callback;
}

void WebServerManager::setSetFractionModeCallback(std::function<bool(const String&)> callback) {
    setFractionModeCallback = callback;
}

void WebServerManager::handleOTA() {
    ArduinoOTA.handle();
}

void WebServerManager::stop() {
    server->end();
    WiFi.disconnect();
}
