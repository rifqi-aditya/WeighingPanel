#ifndef INDICATOR_H
#define INDICATOR_H

#include <Arduino.h>

class Indicator {
public:
    Indicator(uint8_t rxPin, uint8_t txPin);
    void begin(long baudRate = 9600);
    bool update();
    float getWeight() const;
    String getRawData() const;

private:
    uint8_t _rxPin;
    uint8_t _txPin;
    float _currentWeight;
    String _lastRawData;
    String _rxBuffer;
};

#endif
