#include "layouts/LayoutStandard.h"
#include "PrinterManager.h"
#ifdef SUPPORT_USB
#include "USBHostPrinter.h"
#endif

namespace LayoutStandard {
    void print(PrinterManager* printer, String company, String dateTime, float weight, int decimals, int paperSize) {
        int margin = (paperSize == 1) ? 32 : 0; 
        String lineStr = (paperSize == 1) ? "________________________" : "________________";

        if (printer->getPrinterType() == 0 && printer->getSerialPrinter() != nullptr) {
            Adafruit_Thermal* p = printer->getSerialPrinter();
            p->reset();
            delay(100);
            
            // Set Margin
            Serial2.write(0x1D); Serial2.write(0x4C); Serial2.write(margin); Serial2.write(0);

            p->justify('C');
            p->boldOn();
            p->setSize('L');      
            p->println(company);
            p->feed(2); // Tambah 2 baris kosong agar lebih lega
            
            p->boldOff();
            p->setSize('S');      
            p->justify('L');
            p->println("Nama Bahan   : " + lineStr); p->feed(1);
            p->println("Kode Bahan   : " + lineStr); p->feed(1);
            p->println("No. Lot      : " + lineStr); p->feed(1);
            p->println("Nama Product : " + lineStr); p->feed(1);
            p->println("No. Batch    : " + lineStr); p->feed(1);
            
            p->println("Tgl & Jam :");
            p->justify('C');
            p->setSize('L');
            p->println(dateTime);
            p->feed(1);

            p->setSize('S');
            p->justify('L');
            p->println("Berat Bersih :");
            p->justify('C');
            p->boldOn();
            p->setSize('L');
            p->print(String(weight, decimals));
            p->println(" kg");
            
            p->boldOff();
            p->setSize('S');
            p->feed(4);
            
            // Auto-cut
            Serial2.write(0x1D); Serial2.write(0x56); Serial2.write(0x42); Serial2.write(0x00);
        } 
#ifdef SUPPORT_USB
        else if (printer->getPrinterType() == 1 && printer->getUsbPrinter() != nullptr && printer->getUsbPrinter()->isConnected()) {
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

            printer->sendRaw(init, 2);
            printer->sendRaw(setMargin, 4);
            printer->sendRaw(center, 3);
            printer->sendRaw(boldOn, 3);
            printer->sendRaw(sizeLarge, 3);
            printer->sendRaw(company.c_str()); printer->sendRaw("\n\n");

            printer->sendRaw(sizeNormal, 3);
            printer->sendRaw(boldOff, 3);
            printer->sendRaw(left, 3);
            printer->sendRaw("Nama Bahan   : "); printer->sendRaw(lineStr.c_str()); printer->sendRaw("\n\n");
            printer->sendRaw("Kode Bahan   : "); printer->sendRaw(lineStr.c_str()); printer->sendRaw("\n\n");
            printer->sendRaw("No. Lot      : "); printer->sendRaw(lineStr.c_str()); printer->sendRaw("\n\n");
            printer->sendRaw("Nama Product : "); printer->sendRaw(lineStr.c_str()); printer->sendRaw("\n\n");
            printer->sendRaw("No. Batch    : "); printer->sendRaw(lineStr.c_str()); printer->sendRaw("\n\n");

            printer->sendRaw("Tgl & Jam :\n");
            printer->sendRaw(center, 3);
            printer->sendRaw(sizeLarge, 3);
            printer->sendRaw(dateTime.c_str()); printer->sendRaw("\n\n");

            printer->sendRaw(sizeNormal, 3);
            printer->sendRaw(left, 3);
            printer->sendRaw("Berat Bersih :\n");
            printer->sendRaw(center, 3);
            printer->sendRaw(boldOn, 3);
            printer->sendRaw(sizeLarge, 3);
            printer->sendRaw(String(weight, decimals).c_str()); printer->sendRaw(" kg\n");

            printer->sendRaw(boldOff, 3);
            printer->sendRaw(sizeNormal, 3);
            printer->sendRaw(feed3, 3);
            printer->sendRaw(cut, 4);
        }
#endif
    }
}
