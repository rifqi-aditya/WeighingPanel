#ifndef PROTOCOL_A12E_H
#define PROTOCOL_A12E_H

#include <Arduino.h>

namespace ProtocolA12E {
    // Mengirim perintah "R" untuk meminta berat (Command Mode)
    void requestWeight(Stream& serial);
    
    // Melakukan parsing data format "ww001234kg"
    bool parse(String raw, float& weight);
}

#endif
