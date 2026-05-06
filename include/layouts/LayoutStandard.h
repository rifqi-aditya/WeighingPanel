#ifndef LAYOUT_STANDARD_H
#define LAYOUT_STANDARD_H

#include <Arduino.h>

// Forward declaration of PrinterManager to avoid circular dependency
class PrinterManager;

namespace LayoutStandard {
    void print(PrinterManager* printer, String company, String dateTime, float weight, int decimals, int paperSize);
}

#endif
