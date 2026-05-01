#ifndef PRINTER_MANAGER_H
#define PRINTER_MANAGER_H

#include <Adafruit_Thermal.h>
#include <HardwareSerial.h>
#include "USBHostPrinter.h"

class PrinterManager {
    public:
        PrinterManager();
        
        // Inisialisasi Serial2 (jika mode serial) atau USB Host
        bool begin(int type = 0, int paperSize = 0, long baudRate = 115200, int rxPin = 15, int txPin = 16);
        
        // Fungsi Utama: Cetak Tiket Timbangan
        void printTicket(String company, String dateTime, float weight, int decimals = 2);
        
        // Test Print
        void testPrint();

        // Handle USB Events (panggil di main loop)
        void update();

    private:
        Adafruit_Thermal* _serialPrinter;
        USBHostPrinter* _usbPrinter;
        int _printerType; // 0: Serial, 1: USB
        int _paperSize;   // 0: 58mm, 1: 80mm

        void _sendRaw(const uint8_t* data, size_t len);
        void _sendRaw(const char* str);
};

#endif
