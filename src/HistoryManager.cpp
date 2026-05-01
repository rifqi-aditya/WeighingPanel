#include "HistoryManager.h"

HistoryManager::HistoryManager() {}

bool HistoryManager::begin() {
    if (!LittleFS.exists(_filename)) {
        File f = LittleFS.open(_filename, "w");
        if (f) {
            f.println("Timestamp,Weight(kg)");
            f.close();
        }
    }
    return true;
}

void HistoryManager::logWeight(float weight, String timestamp) {
    File f = LittleFS.open(_filename, "a");
    if (f) {
        f.print(timestamp);
        f.print(",");
        f.println(weight, 2);
        f.close();
        Serial.println("HistoryManager: Data logged.");
    } else {
        Serial.println("HistoryManager: Failed to open file for logging.");
    }
}

String HistoryManager::getHistory() {
    File f = LittleFS.open(_filename, "r");
    if (!f) return "";
    
    String content = f.readString();
    f.close();
    return content;
}

void HistoryManager::clearHistory() {
    File f = LittleFS.open(_filename, "w");
    if (f) {
        f.println("Timestamp,Weight(kg)");
        f.close();
        Serial.println("HistoryManager: Log cleared.");
    }
}
