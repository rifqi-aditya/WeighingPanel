#ifdef MODE_CONTINUOUS
#include "workflows/FlowContinuous.h"

namespace FlowContinuous {
    static unsigned long lastIndicatorData = 0;
    static unsigned long lastWsSend = 0;
    static bool indicatorWasOnline = true;
    static unsigned long lastPrintPress = 0;

    void update(Indicator& indicator, DisplayManager& display, WebManager& web, 
                PrinterManager& printer, ConfigManager& config, TimeManager& rtc, 
                HistoryManager& history, bool printRequested, bool physicalBtn) {
        
        if (indicator.update()) {
            float weight = indicator.getWeight();
            lastIndicatorData = millis();
            web.setIndicatorStatus(true);
            indicatorWasOnline = true;
            
            if (millis() - lastWsSend > 200) {
                web.broadcastWeight(weight);
                lastWsSend = millis();
                
                Serial.print("[");
                Serial.print(rtc.getFormattedDateTime());
                Serial.print("] BERAT: ");
                Serial.print(weight, config.getConfig().weightDecimals);
                Serial.println(" kg");
            }
            display.showWeight(weight, config.getConfig().weightDecimals);
        }

        if (millis() - lastIndicatorData > 5000) {
            web.setIndicatorStatus(false);
            if (indicatorWasOnline) {
                indicatorWasOnline = false;
                display.showWaitingData();
                Serial.println("System: Indicator timeout. Waiting for data...");
            }
        }

        // Handle Print Request with non-blocking debounce
        if (printRequested && (millis() - lastPrintPress > 1500)) {
            lastPrintPress = millis();
            Serial.println("System: Processing standard print ticket...");
            float w = indicator.getWeight();
            printer.printTicket(config.getConfig().companyName, rtc.getFullDateTime(), w, 0.0f, w, config.getConfig().weightDecimals);
            
            history.logWeight(w, rtc.getFormattedDateTime());
            Serial.println("System: Print complete.");
        }
    }
}
#endif
