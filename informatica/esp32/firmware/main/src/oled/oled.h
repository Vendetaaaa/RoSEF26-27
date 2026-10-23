#pragma once

#include <Arduino.h>

namespace rosef {

bool oledBegin();
bool oledAvailable();
uint8_t oledAddress();
void oledShowBoot();
void oledShowCommandRunning(const char *command);
void oledShowCommandResult(bool success);
void oledShowTest();

}
