#include "Indicator.h"
#include "protocols/ProtocolA12E.h"

Indicator::Indicator(uint8_t rxPin, uint8_t txPin) : _rxPin(rxPin), _txPin(txPin), _currentWeight(0) {}

void Indicator::begin(long baudRate) {
    Serial1.begin(baudRate, SERIAL_8N1, _rxPin, _txPin);
}

void Indicator::requestWeight() {
#ifdef USE_A12E_PROTOCOL
    ProtocolA12E::requestWeight(Serial1);
#endif
    _rxBuffer = "";
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
#ifdef USE_A12E_PROTOCOL
                if (ProtocolA12E::parse(rawData, _currentWeight)) {
                    newDataAvailable = true;
                } else {
                    Serial.println("System: Protocol A12E Parse Failed: [" + rawData + "]");
                }
#endif
            }
        } else {
            if (_rxBuffer.length() < 100) {
                _rxBuffer += c;
            } else {
                Serial.println("System: RxBuffer Overflow: [" + _rxBuffer + "]");
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

