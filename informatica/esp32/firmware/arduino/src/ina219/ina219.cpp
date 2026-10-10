#include "ina219.h"

#include <Wire.h>

#include "../core/config.h"

namespace rosef {

namespace {

Adafruit_INA219 *g_ina219 = nullptr;
bool g_ina219_ready = false;
bool g_wire_ready = false;
uint8_t g_ina219_address = 0;

void startWire() {
  if (g_wire_ready) {
    return;
  }

  Wire.begin(INA219_SDA_GPIO, INA219_SCL_GPIO);
  Wire.setClock(100000);
  delay(10);
  g_wire_ready = true;
}

bool addressResponds(uint8_t address) {
  Wire.beginTransmission(address);
  return Wire.endTransmission() == 0;
}

bool detectIna219() {
  for (uint8_t address = 0x40; address <= 0x4F; ++address) {
    if (!addressResponds(address)) {
      continue;
    }

    Adafruit_INA219 *candidate = new Adafruit_INA219(address);
    if (candidate == nullptr) {
      continue;
    }

    if (!candidate->begin(&Wire)) {
      delete candidate;
      continue;
    }

    candidate->setCalibration_32V_2A();

    if (!candidate->success()) {
      delete candidate;
      continue;
    }

    if (g_ina219 != nullptr) {
      delete g_ina219;
    }

    g_ina219 = candidate;
    g_ina219_address = address;
    g_ina219_ready = true;
    return true;
  }

  g_ina219_ready = false;
  g_ina219_address = 0;
  return false;
}

}

void ina219Begin() {
  g_ina219_ready = false;
  g_ina219_address = 0;

  if (g_ina219 != nullptr) {
    delete g_ina219;
    g_ina219 = nullptr;
  }

  g_wire_ready = false;
  startWire();
  detectIna219();
}

bool ina219FindOnce() {
  if (g_ina219_ready && g_ina219 != nullptr) {
    return true;
  }

  startWire();
  return detectIna219();
}

bool ina219Available() {
  return g_ina219_ready && g_ina219 != nullptr;
}

uint8_t ina219Address() {
  return g_ina219_address;
}

Ina219Reading ina219Read() {
  Ina219Reading reading{};

  if (!ina219Available()) {
    return reading;
  }

  reading.bus_voltage_v = g_ina219->getBusVoltage_V();
  if (!g_ina219->success()) {
    g_ina219_ready = false;
    g_ina219_address = 0;
    return Ina219Reading{};
  }

  reading.shunt_voltage_mv = g_ina219->getShuntVoltage_mV();
  if (!g_ina219->success()) {
    g_ina219_ready = false;
    g_ina219_address = 0;
    return Ina219Reading{};
  }

  reading.current_ma = g_ina219->getCurrent_mA();
  if (!g_ina219->success()) {
    g_ina219_ready = false;
    g_ina219_address = 0;
    return Ina219Reading{};
  }

  reading.power_mw = g_ina219->getPower_mW();
  if (!g_ina219->success()) {
    g_ina219_ready = false;
    g_ina219_address = 0;
    return Ina219Reading{};
  }

  reading.valid = true;
  return reading;
}

} 
