#include "Indicator.h"

Indicator::Indicator(uint8_t rxPin, uint8_t txPin) : _rxPin(rxPin), _txPin(txPin), _currentWeight(0) {}

void Indicator::begin(long baudRate) {
    Serial1.begin(baudRate, SERIAL_8N1, _rxPin, _txPin);
}

bool Indicator::update() {
    bool newDataAvailable = false;
    while (Serial1.available()) {
        char c = Serial1.read();
        if (c == '\n') {
            String rawData = _rxBuffer;
            _rxBuffer = ""; // Reset buffer
            rawData.trim();
            _lastRawData = rawData;

            if (rawData.length() > 0) {
                int startIdx = rawData.indexOf("ww");
                int endIdx = rawData.indexOf("kg");

                if (startIdx != -1 && endIdx != -1 && endIdx > startIdx) {
                    String weightStr = rawData.substring(startIdx + 2, endIdx);
                    _currentWeight = weightStr.toFloat();
                    newDataAvailable = true;
                }
            }
        } else {
            if (_rxBuffer.length() < 100) {
                _rxBuffer += c;
            } else {
                _rxBuffer = ""; // Clear if too long without newline
            }
        }
    }
    return newDataAvailable;
}

float Indicator::getWeight() const {
    return _currentWeight;
}

String Indicator::getRawData() const {
    return _lastRawData;
}
