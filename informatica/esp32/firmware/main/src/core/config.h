#pragma once

#include <Arduino.h>

namespace rosef {

constexpr uint32_t SERIAL_BAUD = 115200;

constexpr uint8_t TRIGGER_GPIO = 4;
constexpr uint8_t BUTTON_GPIO = 27;

 

constexpr uint8_t RGB_LED_GPIO = 25;
constexpr uint16_t RGB_LED_COUNT = 1;


constexpr uint8_t OLED_SDA_GPIO = 18;
constexpr uint8_t OLED_SCL_GPIO = 19;
constexpr uint8_t OLED_I2C_ADDRESS_PRIMARY = 0x3C;
constexpr uint8_t OLED_I2C_ADDRESS_ALTERNATE = 0x3D;

constexpr uint8_t INA219_SDA_GPIO = 21;
constexpr uint8_t INA219_SCL_GPIO = 22;
constexpr uint8_t INA219_I2C_ADDRESS = 0x40;

 
 
 
constexpr uint8_t TRIGGER_IDLE_LEVEL = HIGH;
constexpr uint8_t TRIGGER_ACTIVE_LEVEL = LOW;

constexpr size_t MAX_SERIAL_LINE = 256;

constexpr uint32_t DEFAULT_TRIGGER_PULSE_US = 1000;

constexpr uint32_t MAX_BENCH_COUNT = 1000;
constexpr uint32_t MAX_BATCH_COUNT = 1000;

}
