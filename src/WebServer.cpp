#include "WebServer.h"
#include "config.h"

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
    // Главная страница
    server->on("/", HTTP_GET, [this](AsyncWebServerRequest *request) {
        request->send(200, "text/html", generateHomePage());
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

    // API: Калибровка воды
    server->on("/api/calibrate/water", HTTP_POST, [](AsyncWebServerRequest *request) {
        // Этот эндпоинт будет вызывать калибровку через callback
        request->send(200, "application/json", "{\"status\":\"calibration_started\",\"step\":\"water\"}");
    });

    // API: Калибровка спирта
    server->on("/api/calibrate/alcohol", HTTP_POST, [](AsyncWebServerRequest *request) {
        request->send(200, "application/json", "{\"status\":\"calibration_started\",\"step\":\"alcohol\"}");
    });

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

String WebServerManager::generateHomePage() {
    String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Smart Areometr</title>
    <style>
        body { font-family: Arial; margin: 20px; background: #f0f0f0; }
        .container { max-width: 600px; margin: 0 auto; background: white; padding: 20px; border-radius: 10px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }
        h1 { color: #333; text-align: center; }
        .measurement { background: #e3f2fd; padding: 20px; border-radius: 8px; margin: 20px 0; text-align: center; }
        .value { font-size: 48px; font-weight: bold; color: #1976d2; }
        .label { font-size: 18px; color: #666; margin-top: 10px; }
        .info { display: flex; justify-content: space-around; margin: 20px 0; }
        .info-item { text-align: center; }
        .info-value { font-size: 24px; font-weight: bold; color: #555; }
        .info-label { font-size: 14px; color: #888; }
        button { background: #1976d2; color: white; border: none; padding: 12px 24px; border-radius: 5px; cursor: pointer; font-size: 16px; margin: 5px; }
        button:hover { background: #1565c0; }
        .status { padding: 10px; background: #c8e6c9; border-radius: 5px; margin: 10px 0; }
    </style>
</head>
<body>
    <div class="container">
        <h1>🍺 Smart Areometr</h1>

        <div class="measurement">
            <div class="value" id="alcohol">--</div>
            <div class="label">Alcohol %</div>
        </div>

        <div class="info">
            <div class="info-item">
                <div class="info-value" id="temperature">--</div>
                <div class="info-label">Temperature °C</div>
            </div>
            <div class="info-item">
                <div class="info-value" id="status">--</div>
                <div class="info-label">Status</div>
            </div>
        </div>

        <div style="text-align: center;">
            <button onclick="calibrateWater()">Calibrate Water</button>
            <button onclick="calibrateAlcohol()">Calibrate Alcohol</button>
            <button onclick="refresh()">Refresh</button>
        </div>

        <div class="status" id="statusMsg">Ready</div>
    </div>

    <script>
        function updateData() {
            fetch('/api/measurement')
                .then(r => r.json())
                .then(data => {
                    document.getElementById('alcohol').textContent = data.alcohol.toFixed(1);
                    document.getElementById('temperature').textContent = data.temperature.toFixed(1);
                    document.getElementById('status').textContent = data.calibrated ? 'OK' : 'Not Cal';
                })
                .catch(e => console.error('Error:', e));
        }

        function calibrateWater() {
            fetch('/api/calibrate/water', {method: 'POST'})
                .then(r => r.json())
                .then(data => {
                    document.getElementById('statusMsg').textContent = 'Water calibration started';
                });
        }

        function calibrateAlcohol() {
            fetch('/api/calibrate/alcohol', {method: 'POST'})
                .then(r => r.json())
                .then(data => {
                    document.getElementById('statusMsg').textContent = 'Alcohol calibration started';
                });
        }

        function refresh() {
            updateData();
            document.getElementById('statusMsg').textContent = 'Data refreshed';
        }

        // Auto-update every 2 seconds
        setInterval(updateData, 2000);
        updateData();
    </script>
</body>
</html>
)rawliteral";
    return html;
}

String WebServerManager::generateMeasurementJSON() {
    JsonDocument doc;

    doc["alcohol"] = alcoholCallback ? alcoholCallback() : 0.0f;
    doc["temperature"] = temperatureCallback ? temperatureCallback() : 0.0f;
    doc["calibrated"] = calibratedCallback ? calibratedCallback() : false;
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

void WebServerManager::handleOTA() {
    ArduinoOTA.handle();
}

void WebServerManager::stop() {
    server->end();
    WiFi.disconnect();
}
