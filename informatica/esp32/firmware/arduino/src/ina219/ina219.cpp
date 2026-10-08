#include "ina219.h"

#include <Wire.h>

#include "../core/config.h"

namespace rosef {

namespace {

Adafruit_INA219 *g_ina219 = nullptr;
bool g_ina219_ready = false;
uint8_t g_ina219_address = 0;

bool addressResponds(uint8_t address) {
  Wire.beginTransmission(address);
  return Wire.endTransmission() == 0;
}

void printReading(const Ina219Reading &reading) {
  if (!reading.valid) {
    Serial.println(F("ERR INA219_UNAVAILABLE"));
    return;
  }

  Serial.print(F("OK INA219 ADDR=0x"));
  if (g_ina219_address < 16) {
    Serial.print('0');
  }
  Serial.print(g_ina219_address, HEX);
  Serial.print(F(" V="));
  Serial.print(reading.bus_voltage_v, 6);
  Serial.print(F(" SHUNT_MV="));
  Serial.print(reading.shunt_voltage_mv, 6);
  Serial.print(F(" I_MA="));
  Serial.print(reading.current_ma, 6);
  Serial.print(F(" P_MW="));
  Serial.println(reading.power_mw, 6);
}

bool detectIna219() {
  for (uint8_t address = 0x40; address <= 0x4F; ++address) {
    if (!addressResponds(address)) {
      continue;
    }

    Adafruit_INA219 *candidate = new Adafruit_INA219(address);

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

  Wire.begin(
      INA219_SDA_GPIO,
      INA219_SCL_GPIO);

  Wire.setClock(100000);
  delay(10);

}

void ina219FindBlocking() {
  g_ina219_ready = false;
  g_ina219_address = 0;

  if (g_ina219 != nullptr) {
    delete g_ina219;
    g_ina219 = nullptr;
  }

  Serial.println(F("INA219 SEARCH"));

  while (!g_ina219_ready) {
    Wire.end();
    delay(20);
    Wire.begin(
        INA219_SDA_GPIO,
        INA219_SCL_GPIO);
    Wire.setClock(100000);
    delay(20);

    if (!detectIna219()) {
      Serial.println(F("INA219 RETRY"));
      delay(250);
      continue;
    }

    Serial.print(F("INA219 DETECTED ADDR=0x"));
    if (g_ina219_address < 16) {
      Serial.print('0');
    }
    Serial.println(g_ina219_address, HEX);

    printReading(ina219Read());
  }
}

bool ina219Available() {
  return g_ina219_ready;
}

uint8_t ina219Address() {
  return g_ina219_address;
}

Ina219Reading ina219Read() {
  Ina219Reading reading{};

  if (!g_ina219_ready || g_ina219 == nullptr) {
    return reading;
  }

  reading.bus_voltage_v =
      g_ina219->getBusVoltage_V();

  if (!g_ina219->success()) {
    g_ina219_ready = false;
    return Ina219Reading{};
  }

  reading.shunt_voltage_mv =
      g_ina219->getShuntVoltage_mV();

  if (!g_ina219->success()) {
    g_ina219_ready = false;
    return Ina219Reading{};
  }

  reading.current_ma =
      g_ina219->getCurrent_mA();

  if (!g_ina219->success()) {
    g_ina219_ready = false;
    return Ina219Reading{};
  }

  reading.power_mw =
      g_ina219->getPower_mW();

  if (!g_ina219->success()) {
    g_ina219_ready = false;
    return Ina219Reading{};
  }

  reading.valid = true;
  return reading;
}

}
