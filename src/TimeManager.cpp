#include "TimeManager.h"
#include <Wire.h>

TimeManager::TimeManager() {}

bool TimeManager::begin(uint8_t sda, uint8_t scl) {
  Wire1.begin(sda, scl);
  Wire1.setClock(100000); // Set to 100kHz for better stability with DS3231

  if (!_rtc.begin(&Wire1)) {
    return false;
  }

  return true;
}

void TimeManager::update() { _now = _rtc.now(); }

void TimeManager::adjust(uint32_t unixTime) { _rtc.adjust(DateTime(unixTime)); }

String TimeManager::getFormattedDateTime() {
  char buffer[32]; // Increased from 17 to 32 for safety
  // Format: DD-MM-YYYY HH:mm
  // Use snprintf to prevent stack smashing if RTC returns invalid/garbage data
  snprintf(buffer, sizeof(buffer), "%02d-%02d-%04d %02d:%02d", _now.day(),
    _now.month(), _now.year(), _now.hour(), _now.minute());
  return String(buffer);
}

String TimeManager::getFullDateTime() {
  char buffer[32];
  // Format: DD-MM-YYYY HH:mm:ss
  snprintf(buffer, sizeof(buffer), "%02d-%02d-%04d %02d:%02d:%02d", _now.day(),
    _now.month(), _now.year(), _now.hour(), _now.minute(),
    _now.second());
  return String(buffer);
}

bool TimeManager::isRTCValid() const { return _now.isValid(); }
