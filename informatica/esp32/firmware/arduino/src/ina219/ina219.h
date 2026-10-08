#pragma once

#include <Arduino.h>
#include <Adafruit_INA219.h>

namespace rosef {

struct Ina219Reading {
  float bus_voltage_v = 0.0f;
  float shunt_voltage_mv = 0.0f;
  float current_ma = 0.0f;
  float power_mw = 0.0f;
  bool valid = false;
};

void ina219Begin();
void ina219FindBlocking();
bool ina219Available();
Ina219Reading ina219Read();
uint8_t ina219Address();

}
