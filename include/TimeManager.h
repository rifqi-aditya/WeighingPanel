#ifndef TIME_MANAGER_H
#define TIME_MANAGER_H

#include <RTClib.h>

class TimeManager {
public:
    TimeManager();
    bool begin(uint8_t sda, uint8_t scl);
    void update();
    void adjust(uint32_t unixTime);
    String getFormattedDateTime(); // Format: DD-MM-YYYY HH:mm
    String getFullDateTime(); // Format: DD-MM-YYYY HH:mm:ss
    bool isRTCValid() const; // Check if the current time is valid

private:
    RTC_DS3231 _rtc;
    DateTime _now;
};

#endif
