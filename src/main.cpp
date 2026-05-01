#include <Arduino.h>
#include "Indicator.h"
#include "DisplayManager.h"
#include "TimeManager.h"
#include "WebManager.h"
#include "ConfigManager.h"
#include "PrinterManager.h"
#include "HistoryManager.h"
#include <TelnetStream.h>

#define LOG_PRINT(x) { Serial.print(x); TelnetStream.print(x); }
#define LOG_PRINTLN(x) { Serial.println(x); TelnetStream.println(x); }


// Konfigurasi Pin
#define INDICATOR_RX 18
#define INDICATOR_TX 17
#define PRINTER_RX 15
#define PRINTER_TX 16
#define LCD_SDA 8
#define LCD_SCL 9
#define RTC_SDA 4
#define RTC_SCL 5
#define PRINT_BUTTON_PIN 1

// Inisialisasi Objek
Indicator indicator(INDICATOR_RX, INDICATOR_TX);
DisplayManager display(0x27, 16, 2);
TimeManager rtc;
WebManager webManager;
ConfigManager configManager;
PrinterManager printer;
HistoryManager history;


void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 3000);
  
  // 0. Inisialisasi Pin Input
  pinMode(PRINT_BUTTON_PIN, INPUT_PULLUP);

  // 1. Inisialisasi Config (Paling Utama)
  if (configManager.begin()) {
    Serial.println("ConfigManager: Ready");
  }

  // 2. Inisialisasi Modul Hardware
  indicator.begin(9600);
  display.begin(LCD_SDA, LCD_SCL);
  history.begin();
  
  if (rtc.begin(RTC_SDA, RTC_SCL)) {
    Serial.println("RTC DS3231: Ready (Wire1)");
  } else {
    Serial.println("RTC DS3231: FAILED! Check connections on GPIO 4/5.");
    display.showDateTime("RTC ERROR!      ");
  }

  // 3. Inisialisasi Printer
  if (printer.begin(115200, PRINTER_RX, PRINTER_TX)) {
    Serial.println("Printer: Ready (Serial2 @ 115200)");
  }


  // 4. Inisialisasi Jaringan & Web Server
  webManager.begin(&rtc, &configManager, &printer, &display, &history);

  // 5. Inisialisasi OTA Logging
  TelnetStream.begin();

  LOG_PRINTLN("\n==================================");
  LOG_PRINT("SYSTEM READY: ");
  LOG_PRINTLN(configManager.getConfig().companyName);
  LOG_PRINTLN("WiFi: Panel_Timbangan (12345678)");
  LOG_PRINTLN("URL: http://timbangan.local");
  LOG_PRINTLN("==================================\n");
}

void loop() {
  webManager.update(); // Handle DNS requests for Captive Portal

  // Update Jam setiap detik
  static unsigned long lastTimeUpdate = 0;
  if (millis() - lastTimeUpdate > 1000) {
    rtc.update();
    if (rtc.isRTCValid()) {
        display.showDateTime(rtc.getFormattedDateTime());
    } else {
        display.showDateTime("RTC ERROR!      ");
    }
    
    // Broadcast system health ke dashboard setiap 2 detik
    static unsigned long lastStatsBroadcast = 0;
    if (millis() - lastStatsBroadcast > 2000) {
        webManager.broadcastSystemStats();
        lastStatsBroadcast = millis();
    }
    
    lastTimeUpdate = millis();
  }

  // Update data dari indikator
  static unsigned long lastIndicatorData = 0;
  static unsigned long lastWsSend = 0;
  static bool indicatorWasOnline = true;

  if (indicator.update()) {
    float weight = indicator.getWeight();
    lastIndicatorData = millis();
    webManager.setIndicatorStatus(true);
    indicatorWasOnline = true;
    
    // Broadcast berat ke dashboard secara Real-time via WebSocket (Throttled max 5Hz)
    if (millis() - lastWsSend > 200) {
        webManager.broadcastWeight(weight);
        lastWsSend = millis();
        
        // Tampilkan ke Serial Monitor
        LOG_PRINT("[");
        LOG_PRINT(rtc.getFormattedDateTime());
        LOG_PRINT("] BERAT: ");
        int dec = configManager.getConfig().weightDecimals;
        Serial.print(weight, dec);
        TelnetStream.print(weight, dec);
        LOG_PRINTLN(" kg");
    }
    
    // Tampilkan ke LCD Baris 2
    display.showWeight(weight, configManager.getConfig().weightDecimals);
  }

  // Cek Timeout Indikator (5 Detik)
  if (millis() - lastIndicatorData > 5000) {
      webManager.setIndicatorStatus(false);
      if (indicatorWasOnline) {
          indicatorWasOnline = false;
          display.showWaitingData();
          LOG_PRINTLN("System: Indicator timeout. Waiting for data...");
      }
  }

  // Cek Permintaan Cetak (Dari Web atau Tombol Fisik NC)
  static bool lastBtnState = digitalRead(PRINT_BUTTON_PIN);
  bool currentBtnState = digitalRead(PRINT_BUTTON_PIN);
  bool physicalPrintRequested = (currentBtnState == HIGH && lastBtnState == LOW);
  lastBtnState = currentBtnState;

  if (webManager.isPrintRequested() || physicalPrintRequested) {
      if (physicalPrintRequested) {
          LOG_PRINTLN("System: External print button triggered.");
      } else {
          LOG_PRINTLN("System: Web print request received.");
      }

      LOG_PRINTLN("System: Processing print ticket...");
      float weightToLog = indicator.getWeight();
      printer.printTicket(configManager.getConfig().companyName, rtc.getFormattedDateTime(), weightToLog, configManager.getConfig().weightDecimals);
      
      // Simpan ke Log History
      history.logWeight(weightToLog, rtc.getFormattedDateTime());
      
      webManager.clearPrintRequest();
      LOG_PRINTLN("System: Print complete.");
      
      if (physicalPrintRequested) delay(500); // Debounce untuk tombol fisik
  }

  // Heartbeat setiap 10 detik
  static unsigned long lastCheck = 0;
  if (millis() - lastCheck > 10000) {
    if (!Serial1.available()) {
      Serial.println("(System Active: Waiting for indicator data...)");
    }
    lastCheck = millis();
  }
}
