#include "PrinterManager.h"

PrinterManager::PrinterManager() {
    _serialPrinter = nullptr;
    _usbPrinter = nullptr;
    _printerType = 0;
}

bool PrinterManager::begin(int type, int paperSize, long baudRate, int rxPin, int txPin) {
    _printerType = type;
    _paperSize = paperSize;

    if (_printerType == 1) { // USB Mode
        Serial.println("PrinterManager: Initializing USB Host...");
        _usbPrinter = new USBHostPrinter();
        return _usbPrinter->begin();
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
    if (_printerType == 1 && _usbPrinter != nullptr) {
        _usbPrinter->handleEvents();
    }
}

void PrinterManager::_sendRaw(const uint8_t* data, size_t len) {
    if (_printerType == 1 && _usbPrinter != nullptr) {
        _usbPrinter->write(data, len);
    } else if (_serialPrinter != nullptr) {
        Serial2.write(data, len);
    }
}

void PrinterManager::_sendRaw(const char* str) {
    _sendRaw((const uint8_t*)str, strlen(str));
}

void PrinterManager::printTicket(String company, String dateTime, float weight, int decimals) {
    int margin = (_paperSize == 1) ? 32 : 0; // 80mm gets margin, 58mm no margin
    String lineStr = (_paperSize == 1) ? "________________________" : "________________"; // 16 chars for 58mm

    if (_printerType == 0 && _serialPrinter != nullptr) {
        _serialPrinter->reset();
        delay(100);
        
        // Set Margin
        Serial2.write(0x1D); Serial2.write(0x4C); Serial2.write(margin); Serial2.write(0);

        _serialPrinter->justify('C');
        _serialPrinter->boldOn();
        _serialPrinter->setSize('L');      
        _serialPrinter->println(company);
        _serialPrinter->feed(1);
        
        _serialPrinter->boldOff();
        _serialPrinter->setSize('S');      
        _serialPrinter->justify('L');
        _serialPrinter->println("Nama Bahan   : " + lineStr); _serialPrinter->feed(1);
        _serialPrinter->println("Kode Bahan   : " + lineStr); _serialPrinter->feed(1);
        _serialPrinter->println("No. Lot      : " + lineStr); _serialPrinter->feed(1);
        _serialPrinter->println("Nama Product : " + lineStr); _serialPrinter->feed(1);
        _serialPrinter->println("No. Batch    : " + lineStr); _serialPrinter->feed(1);
        
        _serialPrinter->println("Tgl & Jam :");
        _serialPrinter->justify('C');
        _serialPrinter->setSize('L');
        _serialPrinter->println(dateTime);
        _serialPrinter->feed(1);

        _serialPrinter->setSize('S');
        _serialPrinter->justify('L');
        _serialPrinter->println("Berat Bersih :");
        _serialPrinter->justify('C');
        _serialPrinter->boldOn();
        _serialPrinter->setSize('L');
        _serialPrinter->print(String(weight, decimals));
        _serialPrinter->println(" kg");
        
        _serialPrinter->boldOff();
        _serialPrinter->setSize('S');
        _serialPrinter->feed(4);
        
        // Auto-cut
        Serial2.write(0x1D); Serial2.write(0x56); Serial2.write(0x42); Serial2.write(0x00);
    } 
    else if (_printerType == 1 && _usbPrinter != nullptr && _usbPrinter->isConnected()) {
        uint8_t init[] = {0x1B, 0x40};
        uint8_t setMargin[] = {0x1D, 0x4C, (uint8_t)margin, 0x00};
        uint8_t center[] = {0x1B, 0x61, 0x01};
        uint8_t left[] = {0x1B, 0x61, 0x00};
        uint8_t boldOn[] = {0x1B, 0x45, 0x01};
        uint8_t boldOff[] = {0x1B, 0x45, 0x00};
        uint8_t sizeLarge[] = {0x1B, 0x21, 0x10};
        uint8_t sizeNormal[] = {0x1B, 0x21, 0x00};
        uint8_t feed3[] = {0x1B, 0x64, 0x03};
        uint8_t cut[] = {0x1D, 0x56, 0x42, 0x00};

        _sendRaw(init, 2);
        _sendRaw(setMargin, 4);
        _sendRaw(center, 3);
        _sendRaw(boldOn, 3);
        _sendRaw(sizeLarge, 3);
        _sendRaw(company.c_str()); _sendRaw("\n\n");

        _sendRaw(sizeNormal, 3);
        _sendRaw(boldOff, 3);
        _sendRaw(left, 3);
        _sendRaw("Nama Bahan   : "); _sendRaw(lineStr.c_str()); _sendRaw("\n\n");
        _sendRaw("Kode Bahan   : "); _sendRaw(lineStr.c_str()); _sendRaw("\n\n");
        _sendRaw("No. Lot      : "); _sendRaw(lineStr.c_str()); _sendRaw("\n\n");
        _sendRaw("Nama Product : "); _sendRaw(lineStr.c_str()); _sendRaw("\n\n");
        _sendRaw("No. Batch    : "); _sendRaw(lineStr.c_str()); _sendRaw("\n\n");

        _sendRaw("Tgl & Jam :\n");
        _sendRaw(center, 3);
        _sendRaw(sizeLarge, 3);
        _sendRaw(dateTime.c_str()); _sendRaw("\n\n");

        _sendRaw(sizeNormal, 3);
        _sendRaw(left, 3);
        _sendRaw("Berat Bersih :\n");
        _sendRaw(center, 3);
        _sendRaw(boldOn, 3);
        _sendRaw(sizeLarge, 3);
        _sendRaw(String(weight, decimals).c_str()); _sendRaw(" kg\n");

        _sendRaw(boldOff, 3);
        _sendRaw(sizeNormal, 3);
        _sendRaw(feed3, 3);
        _sendRaw(cut, 4);
    }
}

void PrinterManager::testPrint() {
    if (_printerType == 1 && _usbPrinter != nullptr && _usbPrinter->isConnected()) {
        _sendRaw("\x1B\x40\x1B\x61\x01 PRINTER USB READY \n\n\n\n\x1D\x56\x42\x00");
    } else if (_printerType == 0 && _serialPrinter != nullptr) {
        _serialPrinter->justify('C');
        _serialPrinter->println("PRINTER SERIAL READY");
        _serialPrinter->feed(3);
    }
}
