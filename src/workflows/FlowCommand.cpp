#ifdef MODE_COMMAND
#include "workflows/FlowCommand.h"

namespace FlowCommand {
    static int weighingState = 0; // 0: IDLE_TARE, 1: WAIT_TARE, 2: IDLE_GROSS, 3: WAIT_GROSS
    static float currentTare = 0;
    static float currentGross = 0;
    static float currentNetto = 0;
    static unsigned long requestTime = 0;
    static unsigned long displayTimer = 0;
    static bool isDisplayingStatus = false;
    static int lastDisplayedState = -1;
    static unsigned long lastTriggerPress = 0;

    void update(Indicator& indicator, DisplayManager& display, WebManager& web, 
                PrinterManager& printer, ConfigManager& config, TimeManager& rtc, 
                HistoryManager& history, bool triggerNext, bool physicalBtn) {
        
        // Timer Non-Blocking untuk Tampilan Status LCD (3 detik)
        if (isDisplayingStatus && millis() - displayTimer > 3000) {
            isDisplayingStatus = false;
            lastDisplayedState = -1; // Paksa refresh tampilan LCD ke state normal
        }

        // Update LCD jika sedang tidak menampilkan status sukses/error
        if (!isDisplayingStatus && lastDisplayedState != weighingState) {
            if (weighingState == 0) display.showCustomLine2("Tekan utk Tare  ");
            else if (weighingState == 1) display.showCustomLine2("Mengambil Tare..");
            else if (weighingState == 2) {
                String tStr = "T:" + String(currentTare, config.getConfig().weightDecimals) + " G:Tekan";
                display.showCustomLine2(tStr);
            }
            else if (weighingState == 3) display.showCustomLine2("Mengambil Gross.");
            lastDisplayedState = weighingState;
        }

        // Handle Indicator update
        if (indicator.update()) {
            web.setIndicatorStatus(true);
            float liveWeight = indicator.getWeight();

            if (weighingState == 1) {
                currentTare = liveWeight;
                web.broadcastCommandData(0, currentTare, 0, weighingState); 
                Serial.println("System: Tare captured: " + String(currentTare));
                weighingState = 2; 
                web.broadcastCommandData(0, currentTare, 0, weighingState); 
            } else if (weighingState == 3) {
                currentGross = liveWeight;
                currentNetto = currentGross - currentTare;
                web.broadcastCommandData(currentGross, currentTare, currentNetto, 4); // State Printing (UI)

                Serial.println("System: Processing print ticket...");
                printer.printTicket(config.getConfig().companyName, rtc.getFullDateTime(), currentGross, currentTare, currentNetto, config.getConfig().weightDecimals);
                
                history.logWeight(currentNetto, rtc.getFormattedDateTime());
                Serial.println("System: Print complete.");
                
                // Set Tampilan Sukses (LCD akan kembali normal setelah timer habis)
                display.showCustomLine2("Tiket Dicetak!  ");
                displayTimer = millis();
                isDisplayingStatus = true;
                
                // Reset State untuk penimbangan baru
                weighingState = 0; 
                currentTare = 0; currentGross = 0; currentNetto = 0;
            }
        }

        // Handle Trigger dengan Debounce Non-Blocking (500ms)
        if (triggerNext && (millis() - lastTriggerPress > 500)) {
            lastTriggerPress = millis();
            if (weighingState == 0) {
                Serial.println("System: Requesting Tare...");
                indicator.requestWeight();
                requestTime = millis();
                weighingState = 1;
                web.broadcastCommandData(0, 0, 0, weighingState);
            } else if (weighingState == 2) {
                Serial.println("System: Requesting Gross...");
                indicator.requestWeight();
                requestTime = millis();
                weighingState = 3;
                web.broadcastCommandData(currentGross, currentTare, currentNetto, weighingState);
            }
        }

        // Handle Timeout (Non-Blocking)
        if ((weighingState == 1 || weighingState == 3) && millis() - requestTime > 5000) {
            Serial.println("System: Indicator timeout!");
            web.setIndicatorStatus(false);
            display.showCustomLine2("Timeout!        ");
            displayTimer = millis();
            isDisplayingStatus = true;
            if (weighingState == 1) weighingState = 0;
            if (weighingState == 3) weighingState = 2;
            lastDisplayedState = -1;
        }
    }
}
#endif
