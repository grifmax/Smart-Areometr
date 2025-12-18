#include "WebServer.h"
#include "config.h"
#include "SerialCompat.h"
#include <LittleFS.h>
#include <Preferences.h>

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
        
        delay(500);
        server->begin();
        delay(500);

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
        
        if (!LittleFS.begin(false)) {
            Serial.println("WARNING: LittleFS not mounted, web interface may not work");
        } else {
            Serial.println("LittleFS mounted for web server");
        }

        setupRoutes();
        setupOTA();
        
        delay(500);
        server->begin();
        delay(500);
        
        Serial.println("Web server started on port " + String(WEB_SERVER_PORT));

        return true;
    } else {
        Serial.println("Failed to create AP");
        return false;
    }
}

void WebServerManager::setupRoutes() {
    // Главная страница
    server->on("/", HTTP_GET, [this](AsyncWebServerRequest *request) {
        if (LittleFS.exists("/index.html")) {
            request->send(LittleFS, "/index.html", "text/html");
        } else {
            const char* html = "<!DOCTYPE html><html><head><title>Smart Areometr</title><meta charset='UTF-8'>"
                "<meta name='viewport' content='width=device-width, initial-scale=1'>"
                "<style>body{font-family:Arial;text-align:center;padding:50px;background:#f0f0f0;}"
                "h1{color:#333;} .info{background:white;padding:20px;border-radius:10px;margin:20px auto;max-width:600px;}"
                "code{background:#f5f5f5;padding:2px 6px;border-radius:3px;}</style></head><body>"
                "<h1>Smart Areometr</h1><div class='info'><p><strong>Веб-интерфейс не загружен в LittleFS.</strong></p>"
                "<p>Выполните: <code>pio run -t uploadfs</code></p>"
                "<p>API: <a href='/api/measurement'>/api/measurement</a> | <a href='/api/status'>/api/status</a></p></div></body></html>";
            request->send(200, "text/html", html);
        }
    });

    // HTML страницы
    server->on("/calibration.html", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (LittleFS.exists("/calibration.html")) {
            request->send(LittleFS, "/calibration.html", "text/html");
        } else {
            request->send(404, "text/plain", "File not found");
        }
    });

    server->on("/settings.html", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (LittleFS.exists("/settings.html")) {
            request->send(LittleFS, "/settings.html", "text/html");
        } else {
            request->send(404, "text/plain", "File not found");
        }
    });

    server->on("/logs.html", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (LittleFS.exists("/logs.html")) {
            request->send(LittleFS, "/logs.html", "text/html");
        } else {
            request->send(404, "text/plain", "File not found");
        }
    });

    server->on("/fractions.html", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (LittleFS.exists("/fractions.html")) {
            request->send(LittleFS, "/fractions.html", "text/html");
        } else {
            request->send(404, "text/plain", "File not found");
        }
    });

    server->on("/mqtt.html", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (LittleFS.exists("/mqtt.html")) {
            request->send(LittleFS, "/mqtt.html", "text/html");
        } else {
            request->send(404, "text/plain", "File not found");
        }
    });

    server->on("/distillation.html", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (LittleFS.exists("/distillation.html")) {
            request->send(LittleFS, "/distillation.html", "text/html");
        } else {
            request->send(404, "text/plain", "File not found");
        }
    });

    // CSS файлы
    server->on("/css/style.css", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (LittleFS.exists("/css/style.css")) {
            request->send(LittleFS, "/css/style.css", "text/css");
        } else {
            request->send(404, "text/plain", "File not found");
        }
    });

    server->on("/css/calibration.css", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (LittleFS.exists("/css/calibration.css")) {
            request->send(LittleFS, "/css/calibration.css", "text/css");
        } else {
            request->send(404, "text/plain", "File not found");
        }
    });

    server->on("/css/logs.css", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (LittleFS.exists("/css/logs.css")) {
            request->send(LittleFS, "/css/logs.css", "text/css");
        } else {
            request->send(404, "text/plain", "File not found");
        }
    });

    // JavaScript файлы
    server->on("/js/app.js", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (LittleFS.exists("/js/app.js")) {
            request->send(LittleFS, "/js/app.js", "application/javascript");
        } else {
            request->send(404, "text/plain", "File not found");
        }
    });

    server->on("/js/calibration.js", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (LittleFS.exists("/js/calibration.js")) {
            request->send(LittleFS, "/js/calibration.js", "application/javascript");
        } else {
            request->send(404, "text/plain", "File not found");
        }
    });

    server->on("/js/settings.js", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (LittleFS.exists("/js/settings.js")) {
            request->send(LittleFS, "/js/settings.js", "application/javascript");
        } else {
            request->send(404, "text/plain", "File not found");
        }
    });

    server->on("/js/logs.js", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (LittleFS.exists("/js/logs.js")) {
            request->send(LittleFS, "/js/logs.js", "application/javascript");
        } else {
            request->send(404, "text/plain", "File not found");
        }
    });

    server->on("/js/fractions.js", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (LittleFS.exists("/js/fractions.js")) {
            request->send(LittleFS, "/js/fractions.js", "application/javascript");
        } else {
            request->send(404, "text/plain", "File not found");
        }
    });

    server->on("/js/mqtt.js", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (LittleFS.exists("/js/mqtt.js")) {
            request->send(LittleFS, "/js/mqtt.js", "application/javascript");
        } else {
            request->send(404, "text/plain", "File not found");
        }
    });

    server->on("/js/distillation.js", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (LittleFS.exists("/js/distillation.js")) {
            request->send(LittleFS, "/js/distillation.js", "application/javascript");
        } else {
            request->send(404, "text/plain", "File not found");
        }
    });

    // Стандартные запросы браузера
    server->on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(204);  // No Content
    });
    
    server->on("/robots.txt", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(200, "text/plain", "User-agent: *\nDisallow: /");
    });

    // API: Получить данные измерений
    server->on("/api/measurement", HTTP_GET, [this](AsyncWebServerRequest *request) {
        String json = generateMeasurementJSON();
        if (json.length() > 0) {
            request->send(200, "application/json", json);
        } else {
            request->send(500, "application/json", "{\"error\":\"internal_error\"}");
        }
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
        if (serializeJson(doc, response) > 0) {
            request->send(200, "application/json", response);
        } else {
            request->send(500, "application/json", "{\"error\":\"serialization_failed\"}");
        }
    });

    // API: Получить системную информацию
    server->on("/api/system/info", HTTP_GET, [](AsyncWebServerRequest *request) {
        JsonDocument doc;
        
        // Свободная память (heap)
        uint32_t freeHeap = ESP.getFreeHeap();
        doc["free_heap"] = freeHeap / 1024;  // В KB
        
        // Uptime в секундах
        unsigned long uptimeSeconds = millis() / 1000;
        doc["uptime"] = uptimeSeconds;
        
        // Информация о файловой системе
        if (LittleFS.begin(false)) {
            size_t totalBytes = LittleFS.totalBytes();
            size_t usedBytes = LittleFS.usedBytes();
            size_t freeBytes = totalBytes - usedBytes;
            
            doc["fs_total"] = totalBytes / 1024;  // В KB
            doc["fs_used"] = usedBytes / 1024;     // В KB
            doc["fs_free"] = freeBytes / 1024;     // В KB
        } else {
            doc["fs_total"] = 0;
            doc["fs_used"] = 0;
            doc["fs_free"] = 0;
        }

        String response;
        if (serializeJson(doc, response) > 0) {
            request->send(200, "application/json", response);
        } else {
            request->send(500, "application/json", "{\"error\":\"serialization_failed\"}");
        }
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
            if (total > 512) {
                request->send(413, "application/json", "{\"error\":\"payload_too_large\"}");
                return;
            }
            
            StaticJsonDocument<256> doc;
            DeserializationError error = deserializeJson(doc, (const char*)data, len);

            if (error || len == 0) {
                request->send(400, "application/json", "{\"error\":\"invalid_json\"}");
                return;
            }

            float alcoholPercent = doc["alcohol_percent"] | 0.0f;
            float temperature = doc["temperature"] | 20.0f;

            if (addCalibrationPointCallback) {
                bool success = addCalibrationPointCallback(alcoholPercent, temperature);
                if (success) {
                    uint16_t rawValue = rawValueCallback ? rawValueCallback() : 0;
                    String response = "{\"status\":\"success\",\"alcohol_percent\":" + String(alcoholPercent) +
                                     ",\"raw_value\":" + String(rawValue) + "}";
                    request->send(200, "application/json", response);
                } else {
                    request->send(500, "application/json", "{\"error\":\"failed_to_add_point\"}");
                }
            } else {
                request->send(500, "application/json", "{\"error\":\"callback_not_set\"}");
            }
        }
    );

    // API: Удалить калибровочную точку
    server->on("/api/calibration/point", HTTP_DELETE, [this](AsyncWebServerRequest *request) {
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
    server->on("/api/fractions/status", HTTP_GET, [this](AsyncWebServerRequest *request) {
        if (getFractionStatusCallback) {
            String json = getFractionStatusCallback();
            request->send(200, "application/json", json);
        } else {
            request->send(500, "application/json", "{\"error\":\"callback_not_set\"}");
        }
    });

    server->on("/api/fractions/stats", HTTP_GET, [this](AsyncWebServerRequest *request) {
        if (getFractionStatsCallback) {
            String json = getFractionStatsCallback();
            request->send(200, "application/json", json);
        } else {
            request->send(500, "application/json", "{\"error\":\"callback_not_set\"}");
        }
    });

    server->on("/api/fractions/thresholds", HTTP_GET, [this](AsyncWebServerRequest *request) {
        if (getFractionThresholdsCallback) {
            String json = getFractionThresholdsCallback();
            request->send(200, "application/json", json);
        } else {
            request->send(500, "application/json", "{\"error\":\"callback_not_set\"}");
        }
    });

    server->on("/api/fractions/thresholds", HTTP_POST,
        [](AsyncWebServerRequest *request) {},
        nullptr,
        [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            if (total > 512) {
                request->send(413, "application/json", "{\"error\":\"payload_too_large\"}");
                return;
            }
            
            if (setFractionThresholdsCallback && len > 0) {
                String body = String((char*)data).substring(0, len);
                bool success = setFractionThresholdsCallback(body);
                if (success) {
                    request->send(200, "application/json", "{\"status\":\"success\"}");
                } else {
                    request->send(400, "application/json", "{\"error\":\"invalid_data\"}");
                }
            } else {
                request->send(400, "application/json", "{\"error\":\"no_body\"}");
            }
        }
    );

    server->on("/api/fractions/reset", HTTP_POST, [this](AsyncWebServerRequest *request) {
        if (resetFractionSessionCallback) {
            resetFractionSessionCallback();
            request->send(200, "application/json", "{\"status\":\"success\"}");
        } else {
            request->send(500, "application/json", "{\"error\":\"callback_not_set\"}");
        }
    });

    server->on("/api/fractions/mode", HTTP_GET, [this](AsyncWebServerRequest *request) {
        if (getFractionModeCallback) {
            String json = getFractionModeCallback();
            request->send(200, "application/json", json);
        } else {
            request->send(500, "application/json", "{\"error\":\"callback_not_set\"}");
        }
    });

    server->on("/api/fractions/mode", HTTP_POST,
        [](AsyncWebServerRequest *request) {},
        nullptr,
        [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            if (total > 512) {
                request->send(413, "application/json", "{\"error\":\"payload_too_large\"}");
                return;
            }
            
            if (setFractionModeCallback && len > 0) {
                String body = String((char*)data).substring(0, len);
                bool success = setFractionModeCallback(body);
                if (success) {
                    request->send(200, "application/json", "{\"status\":\"success\"}");
                } else {
                    request->send(400, "application/json", "{\"error\":\"invalid_data\"}");
                }
            } else {
                request->send(400, "application/json", "{\"error\":\"no_body\"}");
            }
        }
    );

    // === MQTT API ===
    server->on("/api/mqtt/status", HTTP_GET, [this](AsyncWebServerRequest *request) {
        if (getMQTTStatusCallback) {
            String json = getMQTTStatusCallback();
            request->send(200, "application/json", json);
        } else {
            request->send(500, "application/json", "{\"error\":\"callback_not_set\"}");
        }
    });

    server->on("/api/mqtt/config", HTTP_GET, [this](AsyncWebServerRequest *request) {
        if (getMQTTConfigCallback) {
            String json = getMQTTConfigCallback();
            request->send(200, "application/json", json);
        } else {
            request->send(500, "application/json", "{\"error\":\"callback_not_set\"}");
        }
    });

    server->on("/api/mqtt/config", HTTP_POST,
        [](AsyncWebServerRequest *request) {},
        nullptr,
        [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            if (total > 1024) {
                request->send(413, "application/json", "{\"error\":\"payload_too_large\"}");
                return;
            }
            
            if (setMQTTConfigCallback && len > 0) {
                String body = String((char*)data).substring(0, len);
                bool success = setMQTTConfigCallback(body);
                if (success) {
                    request->send(200, "application/json", "{\"status\":\"success\"}");
                } else {
                    request->send(400, "application/json", "{\"error\":\"invalid_config\"}");
                }
            } else {
                request->send(400, "application/json", "{\"error\":\"no_body\"}");
            }
        }
    );

    server->on("/api/mqtt/test", HTTP_POST, [this](AsyncWebServerRequest *request) {
        if (testMQTTCallback) {
            bool success = testMQTTCallback();
            if (success) {
                request->send(200, "application/json", "{\"status\":\"success\",\"connected\":true}");
            } else {
                request->send(200, "application/json", "{\"status\":\"failed\",\"connected\":false}");
            }
        } else {
            request->send(500, "application/json", "{\"error\":\"callback_not_set\"}");
        }
    });

    // === Session API ===
    server->on("/api/session/start", HTTP_POST,
        [](AsyncWebServerRequest *request) {},
        nullptr,
        [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            if (total > 512) {
                request->send(413, "application/json", "{\"error\":\"payload_too_large\"}");
                return;
            }
            
            if (startSessionCallback && len > 0) {
                JsonDocument doc;
                DeserializationError error = deserializeJson(doc, (const char*)data, len);
                
                if (error) {
                    request->send(400, "application/json", "{\"error\":\"invalid_json\"}");
                    return;
                }
                
                String name = doc["name"] | "Session";
                float mashVol = doc["mash_volume"] | 0.0f;
                
                bool success = startSessionCallback(name, mashVol);
                if (success) {
                    request->send(200, "application/json", "{\"status\":\"success\"}");
                } else {
                    request->send(400, "application/json", "{\"error\":\"failed_to_start\"}");
                }
            } else {
                request->send(400, "application/json", "{\"error\":\"no_body\"}");
            }
        }
    );

    server->on("/api/session/stop", HTTP_POST, [this](AsyncWebServerRequest *request) {
        if (stopSessionCallback) {
            stopSessionCallback();
            request->send(200, "application/json", "{\"status\":\"success\"}");
        } else {
            request->send(500, "application/json", "{\"error\":\"callback_not_set\"}");
        }
    });

    server->on("/api/session/pause", HTTP_POST, [this](AsyncWebServerRequest *request) {
        if (pauseSessionCallback) {
            pauseSessionCallback();
            request->send(200, "application/json", "{\"status\":\"success\"}");
        } else {
            request->send(500, "application/json", "{\"error\":\"callback_not_set\"}");
        }
    });

    server->on("/api/session/status", HTTP_GET, [this](AsyncWebServerRequest *request) {
        if (getSessionStatusCallback) {
            String json = getSessionStatusCallback();
            request->send(200, "application/json", json);
        } else {
            request->send(500, "application/json", "{\"error\":\"callback_not_set\"}");
        }
    });

    server->on("/api/session/export/json", HTTP_GET, [this](AsyncWebServerRequest *request) {
        if (exportSessionJSONCallback) {
            String json = exportSessionJSONCallback();
            request->send(200, "application/json", json);
        } else {
            request->send(500, "application/json", "{\"error\":\"callback_not_set\"}");
        }
    });

    server->on("/api/session/export/csv", HTTP_GET, [this](AsyncWebServerRequest *request) {
        if (exportSessionCSVCallback) {
            String csv = exportSessionCSVCallback();
            request->send(200, "text/csv", csv);
        } else {
            request->send(500, "application/json", "{\"error\":\"callback_not_set\"}");
        }
    });

    server->on("/api/session/list", HTTP_GET, [this](AsyncWebServerRequest *request) {
        if (getSessionsListCallback) {
            String json = getSessionsListCallback();
            request->send(200, "application/json", json);
        } else {
            request->send(500, "application/json", "{\"error\":\"callback_not_set\"}");
        }
    });

    // === WiFi API ===
    // API: Сканирование WiFi сетей
    server->on("/api/wifi/scan", HTTP_GET, [](AsyncWebServerRequest *request) {
        Serial.println("Starting WiFi scan...");
        
        // Переключаемся в режим STA для сканирования (если не в AP режиме)
        WiFiMode_t currentMode = WiFi.getMode();
        if (currentMode == WIFI_AP || currentMode == WIFI_AP_STA) {
            WiFi.mode(WIFI_AP_STA);  // AP+STA для сканирования
        } else {
            WiFi.mode(WIFI_STA);
        }
        
        // Запускаем сканирование (блокирующее)
        int n = WiFi.scanNetworks();
        
        JsonDocument doc;
        JsonArray networks = doc["networks"].to<JsonArray>();
        
        if (n > 0) {
            for (int i = 0; i < n; i++) {
                JsonObject network = networks.add<JsonObject>();
                network["ssid"] = WiFi.SSID(i);
                network["rssi"] = WiFi.RSSI(i);
                network["encryption"] = (WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? "open" : "encrypted";
                network["channel"] = WiFi.channel(i);
            }
        } else if (n == 0) {
            doc["error"] = "no_networks";
        } else {
            doc["error"] = "scan_failed";
        }
        
        // Восстанавливаем режим
        if (currentMode == WIFI_AP) {
            WiFi.mode(WIFI_AP);
        }
        
        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
        Serial.println("WiFi scan completed: " + String(n) + " networks found");
    });

    // API: Сохранение WiFi конфигурации
    server->on("/api/wifi/config", HTTP_POST,
        [](AsyncWebServerRequest *request) {},
        nullptr,
        [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            if (total > 512) {
                request->send(413, "application/json", "{\"error\":\"payload_too_large\"}");
                return;
            }
            
            JsonDocument doc;
            DeserializationError error = deserializeJson(doc, (const char*)data, len);
            
            if (error || len == 0) {
                request->send(400, "application/json", "{\"error\":\"invalid_json\"}");
                return;
            }
            
            String ssid = doc["ssid"] | "";
            String password = doc["password"] | "";
            
            if (ssid.length() == 0) {
                request->send(400, "application/json", "{\"error\":\"ssid_required\"}");
                return;
            }
            
            // Сохраняем настройки в Preferences
            Preferences preferences;
            preferences.begin("wifi", false);
            preferences.putString("ssid", ssid);
            preferences.putString("password", password);
            preferences.end();
            
            request->send(200, "application/json", "{\"status\":\"success\",\"message\":\"WiFi settings saved. Device will reboot.\"}");
            Serial.println("WiFi config saved: SSID=" + ssid);
            
            // Перезагрузка через 1 секунду (после отправки ответа)
            delay(1000);
            ESP.restart();
        }
    );

    // API: Сохранение параметров измерения
    server->on("/api/settings/measurement", HTTP_POST,
        [](AsyncWebServerRequest *request) {},
        nullptr,
        [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            if (total > 512) {
                request->send(413, "application/json", "{\"error\":\"payload_too_large\"}");
                return;
            }
            
            JsonDocument doc;
            DeserializationError error = deserializeJson(doc, (const char*)data, len);
            
            if (error || len == 0) {
                request->send(400, "application/json", "{\"error\":\"invalid_json\"}");
                return;
            }
            
            int samples = doc["samples"] | 100;
            int delay = doc["delay"] | 10;
            int interval = doc["interval"] | 5;
            
            // Сохраняем настройки в Preferences
            Preferences preferences;
            preferences.begin("measure", false);
            preferences.putInt("samples", samples);
            preferences.putInt("delay", delay);
            preferences.putInt("interval", interval);
            preferences.end();
            
            request->send(200, "application/json", "{\"status\":\"success\"}");
            Serial.println("Measurement settings saved: samples=" + String(samples) + ", delay=" + String(delay) + ", interval=" + String(interval));
        }
    );

    // API: Получение параметров измерения
    server->on("/api/settings/measurement", HTTP_GET, [](AsyncWebServerRequest *request) {
        Preferences preferences;
        preferences.begin("measure", true);
        
        int samples = preferences.getInt("samples", 100);
        int delay = preferences.getInt("delay", 10);
        int interval = preferences.getInt("interval", 5);
        preferences.end();
        
        JsonDocument doc;
        doc["samples"] = samples;
        doc["delay"] = delay;
        doc["interval"] = interval;
        
        String json;
        serializeJson(doc, json);
        request->send(200, "application/json", json);
    });

    // API: Сохранение настроек температурной компенсации
    server->on("/api/settings/temperature", HTTP_POST,
        [](AsyncWebServerRequest *request) {},
        nullptr,
        [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            if (total > 512) {
                request->send(413, "application/json", "{\"error\":\"payload_too_large\"}");
                return;
            }
            
            JsonDocument doc;
            DeserializationError error = deserializeJson(doc, (const char*)data, len);
            
            if (error || len == 0) {
                request->send(400, "application/json", "{\"error\":\"invalid_json\"}");
                return;
            }
            
            float tempReference = doc["temperature_reference"] | 20.0f;
            float tempCoefficient = doc["temperature_coefficient"] | 0.4f;
            
            // Сохраняем настройки в Preferences
            Preferences preferences;
            preferences.begin("temp", false);
            preferences.putFloat("reference", tempReference);
            preferences.putFloat("coefficient", tempCoefficient);
            preferences.end();
            
            request->send(200, "application/json", "{\"status\":\"success\"}");
            Serial.println("Temperature settings saved: reference=" + String(tempReference) + ", coefficient=" + String(tempCoefficient));
        }
    );

    // API: Получение настроек температурной компенсации
    server->on("/api/settings/temperature", HTTP_GET, [](AsyncWebServerRequest *request) {
        Preferences preferences;
        preferences.begin("temp", true);
        
        float tempReference = preferences.getFloat("reference", 20.0f);
        float tempCoefficient = preferences.getFloat("coefficient", 0.4f);
        preferences.end();
        
        JsonDocument doc;
        doc["temperature_reference"] = tempReference;
        doc["temperature_coefficient"] = tempCoefficient;
        
        String json;
        serializeJson(doc, json);
        request->send(200, "application/json", json);
    });

    // 404 handler
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

String WebServerManager::generateMeasurementJSON() {
    JsonDocument doc;

    doc["alcohol"] = alcoholCallback ? alcoholCallback() : 0.0f;
    doc["temperature"] = temperatureCallback ? temperatureCallback() : 0.0f;
    doc["calibrated"] = calibratedCallback ? calibratedCallback() : false;
    doc["raw_value"] = rawValueCallback ? rawValueCallback() : 0;
    doc["stability"] = stabilityCallback ? stabilityCallback() : 0;
    doc["timestamp"] = millis();

    // Добавляем данные о фракциях, если доступны
    if (getFractionStatusCallback) {
        String fractionStatus = getFractionStatusCallback();
        JsonDocument fracDoc;
        if (deserializeJson(fracDoc, fractionStatus) == DeserializationError::Ok) {
            if (fracDoc["current_fraction"].is<String>()) {
                doc["fraction"] = fracDoc["current_fraction"].as<String>();
            }
            if (fracDoc["fraction_color"].is<String>()) {
                doc["fraction_color"] = fracDoc["fraction_color"].as<String>();
            }
            if (fracDoc["alcohol_rate"].is<float>()) {
                doc["alcohol_rate"] = fracDoc["alcohol_rate"].as<float>();
            }
        }
    }

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

void WebServerManager::setGetMQTTStatusCallback(std::function<String()> callback) {
    getMQTTStatusCallback = callback;
}

void WebServerManager::setGetMQTTConfigCallback(std::function<String()> callback) {
    getMQTTConfigCallback = callback;
}

void WebServerManager::setSetMQTTConfigCallback(std::function<bool(const String&)> callback) {
    setMQTTConfigCallback = callback;
}

void WebServerManager::setTestMQTTCallback(std::function<bool()> callback) {
    testMQTTCallback = callback;
}

void WebServerManager::setStartSessionCallback(std::function<bool(const String&, float)> callback) {
    startSessionCallback = callback;
}

void WebServerManager::setStopSessionCallback(std::function<void()> callback) {
    stopSessionCallback = callback;
}

void WebServerManager::setPauseSessionCallback(std::function<void()> callback) {
    pauseSessionCallback = callback;
}

void WebServerManager::setGetSessionStatusCallback(std::function<String()> callback) {
    getSessionStatusCallback = callback;
}

void WebServerManager::setExportSessionJSONCallback(std::function<String()> callback) {
    exportSessionJSONCallback = callback;
}

void WebServerManager::setExportSessionCSVCallback(std::function<String()> callback) {
    exportSessionCSVCallback = callback;
}

void WebServerManager::setGetSessionsListCallback(std::function<String()> callback) {
    getSessionsListCallback = callback;
}

void WebServerManager::handle() {
    // AsyncWebServer обрабатывает запросы автоматически, нужно только OTA
    ArduinoOTA.handle();
}

void WebServerManager::handleOTA() {
    ArduinoOTA.handle();
}

void WebServerManager::stop() {
    server->end();
    WiFi.disconnect();
}
