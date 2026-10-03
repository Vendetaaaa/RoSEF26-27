#pragma once

#include <Arduino.h>

namespace rosef {

constexpr uint32_t SERIAL_BAUD = 115200;

constexpr uint8_t TRIGGER_GPIO = 4;
constexpr uint8_t BUTTON_GPIO = 18;

constexpr size_t MAX_SERIAL_LINE = 256;

constexpr uint32_t DEFAULT_TRIGGER_PULSE_US = 1000;

constexpr uint32_t MAX_BENCH_COUNT = 1000;
constexpr uint32_t MAX_BATCH_COUNT = 1000;

}
