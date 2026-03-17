#pragma once

#include <Arduino.h>

namespace Pins {
static const uint8_t ENC_CS = D2;          // GPIO4
static const uint8_t RELAY = D0;           // GPIO16
static const uint8_t OPEN_LIMIT = D1;      // GPIO5
static const uint8_t CLOSE_LIMIT = 3;      // RX / GPIO3
}  // namespace Pins
