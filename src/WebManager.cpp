#include "WebManager.h"
#include <WiFi.h>
#include <ESPmDNS.h>
#include <esp_system.h>

WebManager::WebManager() : _server(80), _ws("/ws"), _rtc(nullptr), _config(nullptr), _printer(nullptr), _display(nullptr), _history(nullptr), _lastWeight(0.0f) {}


void WebManager::begin(TimeManager* rtc, ConfigManager* config, PrinterManager* printer, DisplayManager* display, HistoryManager* history) {
    _rtc = rtc;
    _config = config;
    _printer = printer;
    _display = display;
    _history = history;


    // 1. Setup WiFi AP
    setupWiFi();

    // 3. Setup MDNS
    if (MDNS.begin("timbangan")) {
        Serial.println("MDNS responder started (http://timbangan.local)");
    }

    // 3b. Setup DNS Server for Captive Portal (Redirect all to AP IP)
    _dnsServer.start(53, "*", WiFi.softAPIP());
    Serial.println("WebManager: DNS Server started (Captive Portal active)");

    // 4. Setup WebSocket
    _ws.onEvent([this](AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
        this->onWsEvent(server, client, type, arg, data, len);
    });
    _server.addHandler(&_ws);

    // 5. Setup Routes
    setupRoutes();

    // 6. Start Server
    _server.begin();
    
    // 7. Initialize Temperature Sensor (ESP32-S3 Legacy API for Arduino 2.x)
    temp_sensor_config_t temp_sensor = TSENS_CONFIG_DEFAULT();
    temp_sensor.dac_offset = TSENS_DAC_L2; 
    esp_err_t err = temp_sensor_set_config(temp_sensor);
    if (err == ESP_OK) {
        temp_sensor_start();
        Serial.println("WebManager: Internal Temp Sensor Ready (Legacy)");
    } else {
        Serial.println("WebManager: Failed to config Temp Sensor");
    }

    // 8. Initialize OTA
    ElegantOTA.setAutoReboot(true);
    ElegantOTA.begin(&_server);
    ElegantOTA.onStart([this]() { this->onOTAStart(); });
    ElegantOTA.onProgress([this](size_t current, size_t final) { this->onOTAProgress(current, final); });
    ElegantOTA.onEnd([this](bool success) { this->onOTAEnd(success); });

    Serial.println("WebManager: Async HTTP server started");
}

void WebManager::update() {
    _dnsServer.processNextRequest();

    // Handle Auto-Restart
    if (_shouldRestart && (millis() - _restartTimer > 2000)) {
        Serial.println("WebManager: [SYSTEM] Restarting device now...");
        delay(100); // Give Serial time to flush
        ESP.restart();
    }
}

void WebManager::setupWiFi() {
    String ssid = _config->getConfig().wifiSsid;
    String pass = _config->getConfig().wifiPassword;

    if (ssid.length() > 0) {
        Serial.print("WebManager: Attempting to connect to WiFi: ");
        Serial.println(ssid);
        WiFi.mode(WIFI_AP_STA);

        if (_config->getConfig().useStaticIp) {
            IPAddress ip, gw, sn;
            if (ip.fromString(_config->getConfig().staticIp) && 
                gw.fromString(_config->getConfig().gateway) && 
                sn.fromString(_config->getConfig().subnet)) {
                WiFi.config(ip, gw, sn);
                Serial.println("WebManager: Using Static IP configuration");
            }
        }

        WiFi.begin(ssid.c_str(), pass.c_str());

        // Tunggu maksimal 10 detik
        unsigned long startAttempt = millis();
        while (WiFi.status() != WL_CONNECTED && millis() - startAttempt < 10000) {
            delay(500);
            Serial.print(".");
        }

        if (WiFi.status() == WL_CONNECTED) {
            Serial.println("\nWebManager: Connected to WiFi!");
            Serial.print("WebManager: IP Address: ");
            Serial.println(WiFi.localIP());
        } else {
            Serial.println("\nWebManager: WiFi connection failed, falling back to AP.");
        }
    }

    // Selalu jalankan AP sebagai fallback atau pendamping
    WiFi.softAP("Panel_Timbangan", "12345678");
    IPAddress IP = WiFi.softAPIP();
    Serial.print("WebManager: AP IP address: ");
    Serial.println(IP);
}

void WebManager::onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
    if (type == WS_EVT_CONNECT) {
        Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
        // Kirim stats awal saat pertama kali konek
        broadcastSystemStats();
    } else if (type == WS_EVT_DISCONNECT) {
        Serial.printf("WebSocket client #%u disconnected\n", client->id());
    } else if (type == WS_EVT_DATA) {
        AwsFrameInfo *info = (AwsFrameInfo*)arg;
        if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
            data[len] = 0;
            String msg = (char*)data;
            if (msg == "print" && _printer != nullptr) {
                Serial.println("WebManager: Receive PRINT command request");
                _printRequested = true; // Tandai untuk diproses di main loop
            }
        }
    }
}


void WebManager::broadcastWeight(float weight) {
    _lastWeight = weight; // Simpan berat terakhir untuk dicetak
    if (_ws.count() > 0) {

        JsonDocument doc;
        doc["type"] = "weight";
        doc["val"] = weight;
        
        String output;
        serializeJson(doc, output);
        _ws.textAll(output);
    }
}

void WebManager::broadcastSystemStats() {
    if (_ws.count() > 0) {
        JsonDocument doc;
        doc["type"] = "stats";
        doc["heap"] = ESP.getFreeHeap();
        doc["heapSize"] = ESP.getHeapSize();
        doc["rssi"] = (WiFi.status() == WL_CONNECTED) ? WiFi.RSSI() : 0;
        doc["uptime"] = millis() / 1000;
        doc["indicator"] = _indicatorOnline;
        doc["temp"] = getChipTemperature();
        doc["reset"] = getResetReason();
        doc["freq"] = getCpuFrequencyMhz();
        doc["rtcTime"] = (_rtc && _rtc->isRTCValid()) ? _rtc->getFormattedDateTime() : "RTC ERROR";
        doc["ip"] = WiFi.localIP().toString();
        
        String output;
        serializeJson(doc, output);
        _ws.textAll(output);
    }
}

// Removed sendLog method
float WebManager::getChipTemperature() {
    float tsens_out = 0;
    temp_sensor_read_celsius(&tsens_out);
    return tsens_out;
}

String WebManager::getResetReason() {
    esp_reset_reason_t reason = esp_reset_reason();
    switch (reason) {
        case ESP_RST_POWERON: return "Power On";
        case ESP_RST_EXT:     return "External Pin";
        case ESP_RST_SW:      return "Software Reset";
        case ESP_RST_PANIC:   return "Exception/Panic";
        case ESP_RST_INT_WDT: return "Watchdog (Interrupt)";
        case ESP_RST_TASK_WDT:return "Watchdog (Task)";
        case ESP_RST_WDT:     return "Watchdog (Other)";
        case ESP_RST_DEEPSLEEP:return "Deep Sleep";
        case ESP_RST_BROWNOUT:return "Brownout (Voltage Drop)";
        case ESP_RST_SDIO:    return "SDIO Reset";
        default:              return "Unknown";
    }
}

void WebManager::setupRoutes() {
    // 1. Serve index.html dari LittleFS (Unprotected)
    _server.on("/", HTTP_GET, [this](AsyncWebServerRequest *request) {
        request->send(LittleFS, "/index.html", "text/html");
    });

    // 2. API Sinkronisasi Waktu
    _server.on("/api/set_time", HTTP_GET, [this](AsyncWebServerRequest *request) {
        if (request->hasParam("unix")) {
            uint32_t unixTime = request->getParam("unix")->value().toInt();
            uint32_t localTime = unixTime + (7 * 3600); // WIB +7
            if (_rtc != nullptr) {
                _rtc->adjust(localTime);
                request->send(200, "text/plain", "OK");
            }
        } else {
            request->send(400, "text/plain", "Invalid Param");
        }
    });

    // 3. API Get Config
    _server.on("/api/config", HTTP_GET, [this](AsyncWebServerRequest *request) {
        JsonDocument doc;
        doc["companyName"] = _config->getConfig().companyName;
        doc["wifiSsid"] = _config->getConfig().wifiSsid;
        doc["useStaticIp"] = _config->getConfig().useStaticIp;
        doc["staticIp"] = _config->getConfig().staticIp;
        doc["gateway"] = _config->getConfig().gateway;
        doc["subnet"] = _config->getConfig().subnet;
        doc["weightDecimals"] = _config->getConfig().weightDecimals;
        
        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
    });

    // 4. API Save Config
    _server.on("/api/config", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL, [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, data, len); 
        if (!error) {
            if (doc["companyName"].is<const char*>()) _config->setCompanyName(doc["companyName"]);
            if (doc["wifiSsid"].is<const char*>() && doc["wifiPassword"].is<const char*>()) {
                _config->setWiFi(doc["wifiSsid"], doc["wifiPassword"]);
            }
            if (doc["useStaticIp"].is<bool>()) {
                _config->setStaticIp(
                    doc["useStaticIp"],
                    doc["staticIp"] | "192.168.1.100",
                    doc["gateway"] | "192.168.1.1",
                    doc["subnet"] | "255.255.255.0"
                );
            }
            if (doc["weightDecimals"].is<int>()) {
                _config->setWeightDecimals(doc["weightDecimals"].as<int>());
            }
            
            if (_config->save()) {
                request->send(200, "application/json", "{\"status\":\"saved\"}");
                
                // Jika ada perubahan SSID (karena key 'wifiSsid' ada di JSON), tandai untuk restart
                if (!doc["wifiSsid"].isNull()) {
                    _shouldRestart = true;
                    _restartTimer = millis();
                    
                    String msg = "WiFi settings updated. System will restart in 2 seconds to apply changes...";
                    Serial.println("WebManager: " + msg);
                }
            } else {
                request->send(500, "application/json", "{\"status\":\"save_failed\"}");
            }
        } else {
            request->send(400, "application/json", "{\"status\":\"error\"}");
        }
    });

    // 4b. API Get History
    _server.on("/api/history", HTTP_GET, [this](AsyncWebServerRequest *request) {
        String history = _history->getHistory();
        request->send(200, "text/csv", history);
    });

    // 4c. API Clear History
    _server.on("/api/history", HTTP_DELETE, [this](AsyncWebServerRequest *request) {
        _history->clearHistory();
        request->send(200, "application/json", "{\"status\":\"cleared\"}");
    });

    // 5. Static files (CSS/JS)
    _server.serveStatic("/", LittleFS, "/");

    // 6. Catch-all untuk Captive Portal (Android/iOS detect)
    _server.onNotFound([](AsyncWebServerRequest *request) {
        String host = request->host();
        IPAddress apIP = WiFi.softAPIP();
        
        // Hanya redirect jika host bukan IP ESP32 (artinya ini request captive portal seperti gstatic.com)
        if (host != apIP.toString() && host != "timbangan.local") {
            request->redirect(String("http://") + apIP.toString() + "/");
        } else {
            // Jika request adalah file CSS/JS yang hilang, kembalikan 404 agar tidak redirect loop
            request->send(404, "text/plain", "File Not Found (Upload File System Image!)");
        }
    });
}

void WebManager::onOTAStart() {
    Serial.println("OTA Update started...");
    if (_display) {
        _display->showMessage("OTA UPDATE", "Starting...");
    }
}

void WebManager::onOTAProgress(size_t current, size_t final) {
    static unsigned long lastUpdate = 0;
    if (millis() - lastUpdate > 500) {
        int progress = (current * 100) / final;
        Serial.printf("OTA Progress: %d%%\n", progress);
        if (_display) {
            _display->showMessage("OTA UPDATE", "Progress: " + String(progress) + "%");
        }
        lastUpdate = millis();
    }
}

void WebManager::onOTAEnd(bool success) {
    if (success) {
        Serial.println("OTA Update finished successfully!");
        if (_display) {
            _display->showMessage("OTA SUCCESS", "Restarting...");
        }
    } else {
        Serial.println("OTA Update failed!");
        if (_display) {
            _display->showMessage("OTA FAILED", "Check Serial");
        }
    }
}
