#include "protocols/ProtocolA12E.h"

namespace ProtocolA12E {
    void requestWeight(Stream& serial) {
        // Clear buffer before requesting to avoid stale data
        while (serial.available()) {
            serial.read();
        }
        // Send command 'R' in ASCII followed by CR LF
        serial.println("R");
    }

    bool parse(String raw, float& weight) {
        int startIdx = raw.indexOf("ww");
        int endIdx = raw.indexOf("kg");

        if (startIdx != -1 && endIdx != -1 && endIdx > startIdx) {
            String weightStr = raw.substring(startIdx + 2, endIdx);
            weight = weightStr.toFloat();
            return true;
        }
        return false;
    }
}
