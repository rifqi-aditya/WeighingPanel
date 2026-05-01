#ifndef HISTORY_MANAGER_H
#define HISTORY_MANAGER_H

#include <Arduino.h>
#include <LittleFS.h>

class HistoryManager {
public:
    HistoryManager();
    bool begin();
    void logWeight(float weight, String timestamp);
    String getHistory();
    void clearHistory();

private:
    const char* _filename = "/history.csv";
    const int _maxEntries = 100;
};

#endif
