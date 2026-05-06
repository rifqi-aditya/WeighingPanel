#include "PrinterManager.h"
#include "layouts/LayoutStandard.h"
#include "layouts/LayoutLogamJaya.h"

PrinterManager::PrinterManager() {
    _serialPrinter = nullptr;
#ifdef SUPPORT_USB
    _usbPrinter = nullptr;
#endif
    _printerType = 0;
}

bool PrinterManager::begin(int type, int paperSize, long baudRate, int rxPin, int txPin) {
    _printerType = type;
    _paperSize = paperSize;

    if (_printerType == 1) { // USB Mode
        #ifdef SUPPORT_USB
            Serial.println("PrinterManager: Initializing USB Host...");
            _usbPrinter = new USBHostPrinter();
            return _usbPrinter->begin();
        #else
            Serial.println("PrinterManager: USB Mode not supported in this version!");
            return false;
        #endif
    } else { // Serial Mode
        pinMode(txPin, OUTPUT);
        digitalWrite(txPin, HIGH); 
        delay(10);

        Serial2.begin(baudRate, SERIAL_8N1, rxPin, txPin);
        while(Serial2.available()) Serial2.read();

        _serialPrinter = new Adafruit_Thermal(&Serial2);
        delay(500);
        _serialPrinter->begin(150); 
        _serialPrinter->setTimes(0, 0);
        return true;
    }
}

void PrinterManager::update() {
#ifdef SUPPORT_USB
    if (_printerType == 1 && _usbPrinter != nullptr) {
        _usbPrinter->handleEvents();
    }
#endif
}

void PrinterManager::sendRaw(const uint8_t* data, size_t len) {
#ifdef SUPPORT_USB
    if (_printerType == 1 && _usbPrinter != nullptr) {
        _usbPrinter->write(data, len);
        return;
    } 
#endif
    if (_serialPrinter != nullptr) {
        Serial2.write(data, len);
    }
}

void PrinterManager::sendRaw(const char* str) {
    sendRaw((const uint8_t*)str, strlen(str));
}

void PrinterManager::printTicket(String company, String dateTime, float gross, float tare, float netto, int decimals) {
#ifdef TICKET_LOGAM_JAYA
    LayoutLogamJaya::print(this, company, dateTime, gross, tare, netto, decimals, _paperSize);
#else
    // Default / Standard layout (gross is used as weight)
    LayoutStandard::print(this, company, dateTime, gross, decimals, _paperSize);
#endif
}

void PrinterManager::testPrint() {
#ifdef SUPPORT_USB
    if (_printerType == 1 && _usbPrinter != nullptr && _usbPrinter->isConnected()) {
        sendRaw("\x1B\x40\x1B\x61\x01 PRINTER USB READY \n\n\n\n\x1D\x56\x42\x00");
    } else 
#endif
        if (_printerType == 0 && _serialPrinter != nullptr) {
            _serialPrinter->justify('C');
            _serialPrinter->println("PRINTER SERIAL READY");
            _serialPrinter->feed(3);
        }
}
