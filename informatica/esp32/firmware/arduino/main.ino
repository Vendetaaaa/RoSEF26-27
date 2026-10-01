#include <Arduino.h>

namespace rosef {

constexpr uint32_t SERIAL_BAUD = 115200;
constexpr uint8_t TRIGGER_GPIO = 4;
constexpr size_t MAX_LINE = 192;
constexpr uint32_t TEST_TRIGGER_US = 1000;

char line[MAX_LINE];
size_t line_length = 0;

void printHelp() {
  Serial.println(F("OK COMMANDS"));
  Serial.println(F("PING"));
  Serial.println(F("INFO"));
  Serial.println(F("STATUS"));
  Serial.println(F("PULSE"));
  Serial.println(F("HELP"));
}

void printInfo() {
  Serial.print(F("OK ROSEF_ESP32_STAGE0 TRIGGER_GPIO="));
  Serial.print(TRIGGER_GPIO);
  Serial.println(F(" SERIAL=115200"));
}

void printStatus() {
  Serial.print(F("OK STATUS UPTIME_MS="));
  Serial.print(millis());
  Serial.print(F(" TRIGGER="));
  Serial.println(digitalRead(TRIGGER_GPIO) ? F("HIGH") : F("LOW"));
}

void triggerPulse() {
  const uint32_t start_us = micros();
  digitalWrite(TRIGGER_GPIO, HIGH);
  delayMicroseconds(TEST_TRIGGER_US);
  digitalWrite(TRIGGER_GPIO, LOW);

  Serial.print(F("OK PULSE WIDTH_US="));
  Serial.print(micros() - start_us);
  Serial.println();
}

void handleLine(char *command) {
  if (strcmp(command, "PING") == 0) {
    Serial.println(F("OK PONG"));
    return;
  }

  if (strcmp(command, "INFO") == 0) {
    printInfo();
    return;
  }

  if (strcmp(command, "STATUS") == 0) {
    printStatus();
    return;
  }

  if (strcmp(command, "PULSE") == 0) {
    triggerPulse();
    return;
  }

  if (strcmp(command, "HELP") == 0) {
    printHelp();
    return;
  }

  if (*command != '\0') {
    Serial.println(F("ERR UNKNOWN_COMMAND"));
  }
}

void readSerial() {
  while (Serial.available() > 0) {
    const int value = Serial.read();
    if (value < 0) {
      return;
    }

    const char c = static_cast<char>(value);

    if (c == '\r' || c == '\n') {
      if (line_length == 0) {
        continue;
      }

      line[line_length] = '\0';
      handleLine(line);
      line_length = 0;
      continue;
    }

    if (line_length + 1 >= MAX_LINE) {
      line_length = 0;
      Serial.println(F("ERR LINE_TOO_LONG"));
      continue;
    }

    line[line_length++] = c;
  }
}

}

void setup() {
  pinMode(rosef::TRIGGER_GPIO, OUTPUT);
  digitalWrite(rosef::TRIGGER_GPIO, LOW);

  Serial.begin(rosef::SERIAL_BAUD);
  delay(250);

  Serial.println(F("READY ROSEF_ESP32_STAGE0"));
  rosef::printInfo();
  rosef::printHelp();
}

void loop() {
  rosef::readSerial();
}
