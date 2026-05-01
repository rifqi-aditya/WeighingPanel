#include "PrinterManager.h"

PrinterManager::PrinterManager() {
    printer = nullptr;
}

bool PrinterManager::begin(long baudRate, int rxPin, int txPin) {
    // 1. Stabilkan Pin TX sebelum Serial aktif untuk mencegah noise
    pinMode(txPin, OUTPUT);
    digitalWrite(txPin, HIGH); 
    delay(10);

    // 2. Inisialisasi HardwareSerial 2 untuk Printer
    Serial2.begin(baudRate, SERIAL_8N1, rxPin, txPin);
    
    // 3. Bersihkan buffer serial jika ada data sampah saat boot
    while(Serial2.available()) Serial2.read();

    // 4. Inisialisasi Objek Printer dengan Serial2
    printer = new Adafruit_Thermal(&Serial2);
    
    // Berikan waktu sejenak agar serial stabil
    delay(500);
    
    // Untuk Epson, kita bisa menggunakan heatTime yang lebih rendah karena printer industrial
    // dan pastikan printer sudah diset ke baudrate yang sama.
    printer->begin(150); 
    printer->setTimes(0, 0); // Hapus delay internal library agar print secepat baudrate
    
    return true;
}

void PrinterManager::printTicket(String company, String dateTime, float weight, int decimals) {
    if (printer == nullptr) return;

    printer->reset();  
    delay(100);

    // Set Left Margin (Sekitar 4-5mm agar tidak terlalu mepet pinggir)
    Serial2.write(0x1D); Serial2.write(0x4C); 
    Serial2.write(32);   Serial2.write(0);    // 32 dots

    // 1. HEADER: Nama PT (Centered, Large)
    printer->justify('C');
    printer->boldOn();
    printer->setSize('L');      
    printer->println(company);
    printer->feed(1);
    
    printer->boldOff();
    printer->setSize('S');      
    
    // 2. BODY: Data Identitas (Garis Underscore untuk Tulis Tangan)
    printer->justify('L');
    printer->println("Nama Bahan   : ________________________");
    printer->feed(1);
    printer->println("Kode Bahan   : ________________________");
    printer->feed(1);
    printer->println("No. Lot      : ________________________");
    printer->feed(1);
    printer->println("Nama Product : ________________________");
    printer->feed(1);
    printer->println("No. Batch    : ________________________");
    printer->feed(1);
    
    // 3. DATE & TIME (Label left, Value center-large)
    printer->justify('L');
    printer->println("Tgl & Jam :");
    printer->feed(1);
    
    printer->justify('C');
    printer->setSize('L');
    printer->println(dateTime);
    printer->feed(1);

    // 4. WEIGHT (Label left, Value center-large)
    printer->setSize('S');
    printer->justify('L');
    printer->println("Berat Bersih :");
    printer->feed(1);
    
    printer->justify('C');
    printer->boldOn();
    printer->setSize('L');
    printer->print(String(weight, decimals));
    printer->println(" kg");
    
    printer->boldOff();
    printer->setSize('S');
    printer->feed(4);
    
    // Auto-cut command for TM-T82X (GS V 66 0)
    Serial2.write(0x1D);
    Serial2.write(0x56);
    Serial2.write(0x42);
    Serial2.write(0x00);
}

void PrinterManager::testPrint() {
    if (printer == nullptr) return;
    
    printer->justify('C');
    printer->println("PRINTER TEST");
    printer->println("ESP32-S3 READY");
    printer->feed(3);
    
    // Manual Cut
    Serial2.write(0x1D);
    Serial2.write(0x56);
    Serial2.write(0x42);
    Serial2.write(0x00);
}
