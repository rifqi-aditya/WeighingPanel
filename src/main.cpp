#include <Arduino.h>
#include "Indicator.h"
#include "DisplayManager.h"
#include "TimeManager.h"
#include "WebManager.h"
#include "ConfigManager.h"
#include "PrinterManager.h"
#include "HistoryManager.h"
#include "workflows/FlowContinuous.h"
#include "workflows/FlowCommand.h"
#include <TelnetStream.h>

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
  
  pinMode(PRINT_BUTTON_PIN, INPUT_PULLUP);

  if (configManager.begin()) {
    Serial.println("ConfigManager: Ready");
  }

  indicator.begin(9600);
  display.begin(LCD_SDA, LCD_SCL);
  history.begin();
  
  if (rtc.begin(RTC_SDA, RTC_SCL)) {
    Serial.println("RTC DS3231: Ready (Wire1)");
  } else {
    Serial.println("RTC DS3231: FAILED! Check connections.");
    display.showDateTime("RTC ERROR!      ");
  }

  int pType = configManager.getConfig().printerType;
  int pSize = configManager.getConfig().paperSize;
  if (printer.begin(pType, pSize, 115200, PRINTER_RX, PRINTER_TX)) {
    Serial.print("Printer: Ready");
  }

  webManager.begin(&rtc, &configManager, &printer, &display, &history);
  TelnetStream.begin();

  Serial.println("\n==================================");
  Serial.print("SYSTEM READY: ");
  Serial.println(configManager.getConfig().companyName);
  Serial.println("==================================\n");
}

void loop() {
  webManager.update(); 
  printer.update();    

  // Update Jam & Stats setiap detik
  static unsigned long lastTimeUpdate = 0;
  if (millis() - lastTimeUpdate > 1000) {
    rtc.update();
    if (rtc.isRTCValid()) {
        display.showDateTime(rtc.getFormattedDateTime());
    } else {
        display.showDateTime("RTC ERROR!      ");
    }
    
    static unsigned long lastStatsBroadcast = 0;
    if (millis() - lastStatsBroadcast > 2000) {
        webManager.broadcastSystemStats();
        lastStatsBroadcast = millis();
    }
    lastTimeUpdate = millis();
  }

  // Cek Tombol Cetak
  static bool lastBtnState = digitalRead(PRINT_BUTTON_PIN);
  bool currentBtnState = digitalRead(PRINT_BUTTON_PIN);
  bool physicalPrintRequested = (currentBtnState == LOW && lastBtnState == HIGH);
  lastBtnState = currentBtnState;
  
  bool triggerNext = webManager.isPrintRequested() || physicalPrintRequested;
  if (triggerNext) {
      webManager.clearPrintRequest();
  }

  // Jalankan Modul Alur Kerja (Workflow) sesuai Mode
#ifdef MODE_COMMAND
  FlowCommand::update(indicator, display, webManager, printer, configManager, rtc, history, triggerNext, physicalPrintRequested);
#else
  FlowContinuous::update(indicator, display, webManager, printer, configManager, rtc, history, triggerNext, physicalPrintRequested);
#endif

  // Heartbeat setiap 10 detik
  static unsigned long lastCheck = 0;
  if (millis() - lastCheck > 10000) {
    if (!Serial1.available()) {
      Serial.println("(System Active: Waiting for indicator data...)");
    }
    lastCheck = millis();
  }
}
