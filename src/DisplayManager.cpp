#include "DisplayManager.h"
#include <Wire.h>

DisplayManager::DisplayManager(uint8_t address, uint8_t cols, uint8_t rows) : _lcd(address, cols, rows) {}

void DisplayManager::begin(uint8_t sda, uint8_t scl) {
    Wire.begin(sda, scl);
    Wire.setClock(100000); // Set to 100kHz for stability
    _lcd.init();
    _lcd.backlight();
    _lcd.setCursor(0, 0);
    _lcd.print("System Loading..");
}

void DisplayManager::showDateTime(String dateTime) {
    _lcd.setCursor(0, 0);
    _lcd.print(dateTime); // Baris 1: DD-MM-YYYY HH:mm
}

void DisplayManager::showWeight(float weight, int decimals) {
    _lcd.setCursor(0, 1);
    _lcd.print("Berat: ");
    _lcd.print(weight, decimals);
    _lcd.print("      "); // Clear tail
}

void DisplayManager::showWaitingData() {
    _lcd.setCursor(0, 1);
    _lcd.print("MENUNGGU DATA.. "); // 16 chars to clear the line
}

void DisplayManager::showMessage(String line1, String line2) {
    _lcd.clear();
    _lcd.setCursor(0, 0);
    _lcd.print(line1.substring(0, 16));
    if (line2.length() > 0) {
        _lcd.setCursor(0, 1);
        _lcd.print(line2.substring(0, 16));
    }
}

void DisplayManager::clear() {
    _lcd.clear();
}
