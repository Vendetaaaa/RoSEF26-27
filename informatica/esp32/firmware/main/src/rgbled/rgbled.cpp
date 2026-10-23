#include "rgbled.h"

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

#include "../core/config.h"

namespace rosef {
namespace {



Adafruit_NeoPixel pixel(
    RGB_LED_COUNT,
    RGB_LED_GPIO,
    NEO_GRB + NEO_KHZ800);

void setColor(uint8_t red, uint8_t green, uint8_t blue) {
  pixel.setPixelColor(0, pixel.Color(red, green, blue));
  pixel.show();
}

}  

void rgbLedBegin() {
  pixel.begin();
  pixel.setBrightness(48);  
  pixel.clear();
  pixel.show();
  rgbLedSetIdle();  
}


void rgbLedSetIdle() { setColor(0, 255, 0); }


void rgbLedSetCommandRunning() { setColor(255, 72, 0); }

void rgbLedSetRunning() { rgbLedSetCommandRunning(); }
void rgbLedSetPass() { rgbLedSetIdle(); }
void rgbLedSetFail() { setColor(255, 0, 0); }
void rgbLedSetOff() { setColor(0, 0, 0); }

}  
