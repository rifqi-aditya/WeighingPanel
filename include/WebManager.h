#ifndef WEB_MANAGER_H
#define WEB_MANAGER_H

#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <DNSServer.h>
#include <driver/temp_sensor.h>  // Legacy API untuk ESP32-S3 pada Arduino Core v2.x
#include "TimeManager.h"
#include "ConfigManager.h"
#include "PrinterManager.h"
#include "DisplayManager.h"
#include "HistoryManager.h"
#include <ElegantOTA.h>


class WebManager {
public:
    WebManager();
    void begin(TimeManager* rtc, ConfigManager* config, PrinterManager* printer, DisplayManager* display, HistoryManager* history);

    // Update DNS processing (panggil di main loop)
    void update();
    
    // Check & Clear print requests
    bool isPrintRequested() { return _printRequested; }
    void clearPrintRequest() { _printRequested = false; }

    // Broadcast data ke semua client yang terhubung
    void broadcastWeight(float weight);
    void broadcastCommandData(float gross, float tare, float netto, int state);
    void broadcastSystemStats();
    void setIndicatorStatus(bool online) { _indicatorOnline = online; }

private:
    AsyncWebServer _server;
    AsyncWebSocket _ws;
    DNSServer _dnsServer;
    TimeManager* _rtc;
    ConfigManager* _config;
    PrinterManager* _printer;
    DisplayManager* _display;
    HistoryManager* _history;
    float _lastWeight;
    bool _printRequested = false;
    bool _shouldRestart = false;
    unsigned long _restartTimer = 0;
    bool _indicatorOnline = false;

    // No handle needed for legacy API
    
    void setupRoutes();
    void setupWiFi();
    void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len);
    
    // Monitoring Helpers
    float getChipTemperature();
    String getResetReason();

    // OTA Callbacks
    void onOTAStart();
    void onOTAProgress(size_t current, size_t final);
    void onOTAEnd(bool success);
};

#endif
