#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <Arduino.h>
#include <LiquidCrystal_I2C.h>

class DisplayManager {
public:
    DisplayManager(uint8_t address = 0x27, uint8_t cols = 16, uint8_t rows = 2);
    void begin(uint8_t sda, uint8_t scl);
    void showDateTime(String dateTime);
    void showWeight(float weight, int decimals = 2);
    void showWaitingData();
    void showCustomLine2(String text);
    void showMessage(String line1, String line2 = "");
    void clear();

private:
    LiquidCrystal_I2C _lcd;
};

#endif
