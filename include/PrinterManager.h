#ifndef PRINTER_MANAGER_H
#define PRINTER_MANAGER_H

#include <Arduino.h>
#include <Adafruit_Thermal.h>
#include <HardwareSerial.h>

class PrinterManager {
    public:
        PrinterManager();
        
        // Inisialisasi Serial2 dan objek printer
        bool begin(long baudRate = 115200, int rxPin = 15, int txPin = 16);
        
        // Fungsi Utama: Cetak Tiket Timbangan
        void printTicket(String company, String dateTime, float weight, int decimals = 2);
        
        // Test Print
        void testPrint();

    private:
        Adafruit_Thermal* printer;
};

#endif
