#include "rgbled.h"

#include <Arduino.h>
#include "../core/config.h"

namespace rosef {
namespace {

void writeChannel(uint8_t pin, bool on) {
  const uint8_t on_level = RGB_LED_COMMON_ANODE ? LOW : HIGH;
  const uint8_t off_level = RGB_LED_COMMON_ANODE ? HIGH : LOW;
  digitalWrite(pin, on ? on_level : off_level);
}

void setColor(bool red, bool green, bool blue) {
  writeChannel(RGB_LED_R_GPIO, red);
  writeChannel(RGB_LED_G_GPIO, green);
  writeChannel(RGB_LED_B_GPIO, blue);
}

}   

void rgbLedBegin() {
  pinMode(RGB_LED_R_GPIO, OUTPUT);
  pinMode(RGB_LED_G_GPIO, OUTPUT);
  pinMode(RGB_LED_B_GPIO, OUTPUT);

   
   
  rgbLedSetOff();
  setColor(true, false, false);
  delay(180);
  setColor(false, true, false);
  delay(180);
  setColor(false, false, true);
  delay(180);
  rgbLedSetOff();
  rgbLedSetIdle();
}

 
void rgbLedSetIdle() { setColor(false, false, true); }

 
void rgbLedSetCommandRunning() { setColor(true, true, false); }

 
void rgbLedSetRunning() { setColor(true, false, true); }

 
void rgbLedSetPass() { setColor(false, true, false); }

 
void rgbLedSetFail() { setColor(true, false, false); }

void rgbLedSetOff() { setColor(false, false, false); }

}   
