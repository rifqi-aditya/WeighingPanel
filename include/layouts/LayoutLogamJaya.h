#ifndef LAYOUT_LOGAM_JAYA_H
#define LAYOUT_LOGAM_JAYA_H

#include <Arduino.h>

class PrinterManager;

namespace LayoutLogamJaya {
    void print(PrinterManager* printer, String company, String dateTime, float gross, float tare, float netto, int decimals, int paperSize);
}

#endif
