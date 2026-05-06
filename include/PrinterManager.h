#ifndef PRINTER_MANAGER_H
#define PRINTER_MANAGER_H

#include <Adafruit_Thermal.h>
#include <HardwareSerial.h>

#ifdef SUPPORT_USB
#include "USBHostPrinter.h"
#endif

class PrinterManager {
    public:
        PrinterManager();
        
        // Inisialisasi Serial2 (jika mode serial) atau USB Host
        bool begin(int type = 0, int paperSize = 0, long baudRate = 115200, int rxPin = 15, int txPin = 16);
        
        // Fungsi Utama: Cetak Tiket Timbangan
        void printTicket(String company, String dateTime, float gross, float tare, float netto, int decimals = 2);
        
        // Test Print
        void testPrint();

        // Handle USB Events (panggil di main loop)
        void update();

        // Public accessors for modular layouts
        void sendRaw(const uint8_t* data, size_t len);
        void sendRaw(const char* str);
        int getPrinterType() const { return _printerType; }
        int getPaperSize() const { return _paperSize; }
        void setPaperSize(int size) { _paperSize = size; }
        Adafruit_Thermal* getSerialPrinter() { return _serialPrinter; }
#ifdef SUPPORT_USB
        USBHostPrinter* getUsbPrinter() { return _usbPrinter; }
#endif

    private:
        Adafruit_Thermal* _serialPrinter;
#ifdef SUPPORT_USB
        USBHostPrinter* _usbPrinter;
#endif
        int _printerType; // 0: Serial, 1: USB
        int _paperSize;   // 0: 58mm, 1: 80mm

        void _sendRaw(const uint8_t* data, size_t len);
        void _sendRaw(const char* str);
};

#endif
