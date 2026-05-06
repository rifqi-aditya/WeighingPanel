#ifndef FLOW_CONTINUOUS_H
#define FLOW_CONTINUOUS_H

#include <Arduino.h>
#include "Indicator.h"
#include "DisplayManager.h"
#include "WebManager.h"
#include "PrinterManager.h"
#include "ConfigManager.h"
#include "TimeManager.h"
#include "HistoryManager.h"

namespace FlowContinuous {
    void update(Indicator& indicator, DisplayManager& display, WebManager& web, 
                PrinterManager& printer, ConfigManager& config, TimeManager& rtc, 
                HistoryManager& history, bool printRequested, bool physicalBtn);
}

#endif
