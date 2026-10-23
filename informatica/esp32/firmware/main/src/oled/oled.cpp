#include "oled.h"

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <cstring>

#include "leakon_logo.h"
#include "../core/config.h"

namespace rosef {
namespace {

constexpr int8_t OLED_RESET_PIN = -1;
constexpr uint32_t OLED_I2C_CLOCK_HZ = 100000;
constexpr uint8_t OLED_WIDTH = 128;
constexpr uint8_t OLED_HEIGHT = 64;

TwoWire oledWire(1);
Adafruit_SSD1306 display(OLED_WIDTH, OLED_HEIGHT, &oledWire, OLED_RESET_PIN);

bool g_bus_started = false;
bool g_oled_ready = false;
uint8_t g_oled_address = 0;
char g_last_command[48] = "BOOT";

bool addressResponds(uint8_t address) {
  oledWire.beginTransmission(address);
  return oledWire.endTransmission() == 0;
}

void printClippedText(const char *text, size_t max_chars) {
  if (!text) {
    display.print(F("NONE"));
    return;
  }
  size_t i = 0;
  while (i < max_chars && text[i] != '\0' && text[i] != '\r' && text[i] != '\n') {
    display.write(static_cast<uint8_t>(text[i]));
    ++i;
  }
  if (text[i] != '\0' && i == max_chars) display.print(F("~"));
}

void rememberCommand(const char *command) {
  if (!command) command = "NONE";
  size_t i = 0;
  while (i < sizeof(g_last_command) - 1 && command[i] != '\0' &&
         command[i] != '\r' && command[i] != '\n') {
    char c = command[i];
    if (c >= 'a' && c <= 'z') c = static_cast<char>(c - ('a' - 'A'));
    g_last_command[i] = c;
    ++i;
  }
  g_last_command[i] = '\0';
}

uint16_t tinyGlyph(char c) {
  static const uint16_t letters[26] = {
    0b010101111101101, 0b110101110101110, 0b011100100100011,
    0b110101101101110, 0b111100110100111, 0b111100110100100,
    0b011100101101011, 0b101101111101101, 0b111010010010111,
    0b001001001101010, 0b101101110101101, 0b100100100100111,
    0b101111111101101, 0b110101101101101, 0b010101101101010,
    0b110101110100100, 0b010101101111011, 0b110101110101101,
    0b011100010001110, 0b111010010010010, 0b101101101101111,
    0b101101101101010, 0b101101111111101, 0b101101010101101,
    0b101101010010010, 0b111001010100111
  };
  static const uint16_t digits[10] = {
    0b111101101101111, 0b010110010010111, 0b110001010100111,
    0b110001010001110, 0b101101111001001, 0b111100110001110,
    0b011100110101010, 0b111001010010010, 0b111101111101111,
    0b010101011001110
  };
  if (c >= 'a' && c <= 'z') c = static_cast<char>(c - ('a' - 'A'));
  if (c >= 'A' && c <= 'Z') return letters[c - 'A'];
  if (c >= '0' && c <= '9') return digits[c - '0'];
  switch (c) {
    case ' ': return 0;
    case '&': return 0b010101010101011;
    case '>': return 0b100010001010100;
    case '<': return 0b001010100010001;
    case '/': return 0b001001010100100;
    case '-': return 0b000000111000000;
    case '.': return 0b000000000000010;
    case ':': return 0b000000010000010;
    case '+': return 0b000010111010000;
    case '_': return 0b000000000000111;
    case '=': return 0b000111000111000;
    case '[': return 0b110100100100110;
    case ']': return 0b011001001001011;
    default: return 0;
  }
}

void drawTinyText(int16_t x, int16_t y, const char *text, uint16_t color = SSD1306_WHITE, uint8_t scale = 1, uint8_t spacing = 1) {
  if (!text) return;
  int16_t cursor_x = x;
  for (size_t i = 0; text[i] != '\0'; ++i) {
    const uint16_t glyph = tinyGlyph(text[i]);
    for (uint8_t row = 0; row < 5; ++row) {
      const uint8_t row_bits = static_cast<uint8_t>((glyph >> ((4 - row) * 3)) & 0x07);
      for (uint8_t col = 0; col < 3; ++col) {
        if (row_bits & (1U << (2 - col))) {
          display.fillRect(cursor_x + col * scale, y + row * scale, scale, scale, color);
        }
      }
    }
    cursor_x += 3 * scale + spacing;
  }
}

void drawTinyCentered(int16_t y, const char *text, uint16_t color = SSD1306_WHITE, uint8_t scale = 1, uint8_t spacing = 1) {
  if (!text) return;
  const int16_t width = static_cast<int16_t>(strlen(text) * (3 * scale + spacing) - spacing);
  drawTinyText((OLED_WIDTH - width) / 2, y, text, color, scale, spacing);
}

void drawMiniMark(int16_t x, int16_t y) {
  display.drawLine(x, y + 7, x + 4, y, SSD1306_WHITE);
  display.drawLine(x + 4, y, x + 8, y + 7, SSD1306_WHITE);
  display.drawFastHLine(x + 2, y + 5, 5, SSD1306_WHITE);
  display.drawCircle(x + 12, y + 4, 3, SSD1306_WHITE);
  display.fillCircle(x + 12, y + 4, 1, SSD1306_WHITE);
}

void drawHeader(const __FlashStringHelper *section) {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setTextWrap(false);
  drawMiniMark(3, 1);
  display.setCursor(21, 2);
  display.print(F("LEAKON"));
  display.drawFastVLine(61, 1, 8, SSD1306_WHITE);
  display.setCursor(66, 2);
  display.print(section);
  display.drawRoundRect(115, 1, 10, 8, 1, SSD1306_WHITE);
  display.fillRect(118, 4, 4, 2, SSD1306_WHITE);
  display.drawFastHLine(0, 10, OLED_WIDTH, SSD1306_WHITE);
}

void drawFooter() {
  display.drawFastHLine(2, 55, 124, SSD1306_WHITE);
  drawTinyText(4, 58, "SIG > LEARN > RECOVER");
  drawTinyText(104, 58, "ESP32");
}

void drawLogo() {
  display.drawBitmap(2, 13, LEAKON_LOGO_BITMAP, LEAKON_LOGO_WIDTH, LEAKON_LOGO_HEIGHT, SSD1306_WHITE);
  display.fillRoundRect(44, 13, 16, 15, 2, SSD1306_WHITE);
  display.drawLine(47, 25, 51, 15, SSD1306_BLACK);
  display.drawLine(51, 15, 56, 25, SSD1306_BLACK);
  display.fillCircle(95, 20, 1, SSD1306_WHITE);
}

void drawPipelineBoxes(int8_t active) {
  const int16_t xs[3] = {3, 45, 87};
  const uint8_t widths[3] = {38, 38, 38};
  const char *labels[3] = {"ECDSA", "CNN1D", "HNP+LLL"};
  for (uint8_t i = 0; i < 3; ++i) {
    if (active >= static_cast<int8_t>(i)) {
      display.fillRoundRect(xs[i], 46, widths[i], 10, 2, SSD1306_WHITE);
      const int16_t text_width = static_cast<int16_t>(strlen(labels[i]) * 4 - 1);
      drawTinyText(xs[i] + (widths[i] - text_width) / 2, 48, labels[i], SSD1306_BLACK);
    } else {
      display.drawRoundRect(xs[i], 46, widths[i], 10, 2, SSD1306_WHITE);
      const int16_t text_width = static_cast<int16_t>(strlen(labels[i]) * 4 - 1);
      drawTinyText(xs[i] + (widths[i] - text_width) / 2, 48, labels[i]);
    }
  }
}

void drawSplash(int8_t active, bool ready) {
  drawHeader(F("BOOT"));
  drawLogo();
  drawTinyCentered(30, "LEAKAGE & NEURONAL ANALYSIS");
  display.drawFastHLine(7, 37, 114, SSD1306_WHITE);
  drawTinyCentered(40, "SIGNAL > LEARNING > RECOVERY");
  drawPipelineBoxes(active);
  display.drawFastHLine(3, 58, 122, SSD1306_WHITE);
  drawTinyText(4, 59, ready ? "SYSTEM READY" : "BOOTSTRAP");
  drawTinyText(96, 59, "NODE 01");
  display.display();
}

void drawBootAnimation() {
  for (int8_t step = 0; step < 3; ++step) {
    drawSplash(step, false);
    delay(82);
  }
}

void drawBootScreen() {
  drawSplash(-1, true);
}

void drawActivityMeter() {
  const uint8_t active = static_cast<uint8_t>((millis() / 180) % 10);
  for (uint8_t i = 0; i < 10; ++i) {
    const int16_t x = 6 + static_cast<int16_t>(i) * 12;
    display.drawRoundRect(x, 49, 8, 4, 1, SSD1306_WHITE);
    if (i < 3 || i == active) display.fillRect(x + 2, 50, 4, 2, SSD1306_WHITE);
  }
}

void drawCommandScreen() {
  drawHeader(F("RUN"));
  drawTinyText(5, 13, "LIVE TRACE / COMMAND");
  display.drawRoundRect(3, 22, 122, 19, 3, SSD1306_WHITE);
  display.fillRoundRect(5, 24, 118, 15, 2, SSD1306_WHITE);
  display.setTextColor(SSD1306_BLACK);
  display.setTextSize(1);
  display.setCursor(9, 28);
  printClippedText(g_last_command, 17);
  display.setTextColor(SSD1306_WHITE);
  drawTinyText(5, 44, "CAPTURE PIPELINE");
  drawActivityMeter();
  drawFooter();
  display.display();
}

void drawResultScreen(bool success) {
  drawHeader(success ? F("PASS") : F("FAULT"));
  drawTinyText(5, 14, success ? "COMMAND COMPLETE" : "COMMAND FAILED");
  display.drawRoundRect(5, 22, 25, 26, 3, SSD1306_WHITE);
  display.drawCircle(17, 34, 8, SSD1306_WHITE);
  if (success) {
    display.drawLine(12, 34, 16, 38, SSD1306_WHITE);
    display.drawLine(16, 38, 23, 29, SSD1306_WHITE);
  } else {
    display.drawLine(12, 29, 22, 39, SSD1306_WHITE);
    display.drawLine(22, 29, 12, 39, SSD1306_WHITE);
  }
  display.fillRoundRect(35, 22, 89, 18, 2, SSD1306_WHITE);
  display.setTextColor(SSD1306_BLACK);
  display.setTextSize(2);
  display.setCursor(success ? 55 : 49, 23);
  display.print(success ? F("DONE") : F("ERROR"));
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(37, 40);
  printClippedText(g_last_command, 14);
  drawTinyText(37, 50, success ? "CYCLE CLOSED" : "CHECK OUTPUT");
  drawFooter();
  display.display();
}

void drawSignalBars(int16_t x, int16_t y) {
  display.drawFastVLine(x, y + 6, 2, SSD1306_WHITE);
  display.fillRect(x + 4, y + 4, 2, 4, SSD1306_WHITE);
  display.fillRect(x + 8, y + 2, 2, 6, SSD1306_WHITE);
  display.fillRect(x + 12, y, 2, 8, SSD1306_WHITE);
}

void drawTestScreen() {
  drawHeader(F("I2C"));
  drawTinyText(5, 14, "SSD1306 / DISPLAY LINK");
  display.drawRoundRect(4, 21, 120, 25, 3, SSD1306_WHITE);
  drawTinyText(10, 23, "DEVICE ADDRESS");
  display.setTextSize(2);
  display.setCursor(10, 28);
  display.print(F("0x"));
  display.print(static_cast<unsigned int>(g_oled_address), HEX);
  display.setTextSize(1);
  display.setCursor(68, 33);
  display.print(F("ACK"));
  drawSignalBars(102, 31);
  drawTinyText(5, 49, "SDA 18 / SCL 19 / BUS 1");
  drawFooter();
  display.display();
}

void printProbe(uint8_t address, bool found) {
  Serial.print(F("OLED_PROBE_0x"));
  Serial.print(static_cast<unsigned int>(address), HEX);
  Serial.print(F("="));
  Serial.println(found ? F("ACK") : F("NO_ACK"));
}

}

bool oledBegin() {
  if (g_oled_ready) return true;
  g_oled_address = 0;
  Serial.println(F("OLED_INIT_START"));
  Serial.println(F("OLED_I2C_CONTROLLER=1"));
  Serial.print(F("OLED_SDA_GPIO="));
  Serial.println(OLED_SDA_GPIO);
  Serial.print(F("OLED_SCL_GPIO="));
  Serial.println(OLED_SCL_GPIO);

  if (!g_bus_started) {
    if (!oledWire.begin(OLED_SDA_GPIO, OLED_SCL_GPIO, OLED_I2C_CLOCK_HZ)) {
      Serial.println(F("OLED_I2C_BEGIN=FAILED"));
      return false;
    }
    g_bus_started = true;
  }

  oledWire.setClock(OLED_I2C_CLOCK_HZ);
  delay(10);
  Serial.println(F("OLED_I2C_BEGIN=OK"));

  const bool found_primary = addressResponds(OLED_I2C_ADDRESS_PRIMARY);
  printProbe(OLED_I2C_ADDRESS_PRIMARY, found_primary);
  const bool found_alternate = addressResponds(OLED_I2C_ADDRESS_ALTERNATE);
  printProbe(OLED_I2C_ADDRESS_ALTERNATE, found_alternate);

  if (found_primary) {
    g_oled_address = OLED_I2C_ADDRESS_PRIMARY;
  } else if (found_alternate) {
    g_oled_address = OLED_I2C_ADDRESS_ALTERNATE;
  } else {
    Serial.println(F("OLED_DEVICE=NOT_FOUND"));
    return false;
  }

  Serial.print(F("OLED_I2C_ADDRESS=0x"));
  Serial.println(static_cast<unsigned int>(g_oled_address), HEX);

  if (!display.begin(SSD1306_SWITCHCAPVCC, g_oled_address, true, false)) {
    Serial.println(F("OLED_SSD1306_INIT=FAILED"));
    g_oled_address = 0;
    return false;
  }

  g_oled_ready = true;
  drawBootAnimation();
  drawBootScreen();
  Serial.println(F("OLED_SSD1306_INIT=OK"));
  Serial.println(F("OLED_DISPLAY_UPDATE=OK"));
  Serial.println(F("OLED_READY=1"));
  return true;
}

bool oledAvailable() { return g_oled_ready; }

uint8_t oledAddress() { return g_oled_address; }

void oledShowBoot() {
  if (!g_oled_ready) return;
  drawBootScreen();
}

void oledShowCommandRunning(const char *command) {
  rememberCommand(command);
  if (!g_oled_ready) return;
  drawCommandScreen();
}

void oledShowTest() {
  if (!g_oled_ready) return;
  drawTestScreen();
}

void oledShowCommandResult(bool success) {
  if (!g_oled_ready) return;
  if (success && strcmp(g_last_command, "OLEDTEST") == 0) {
    oledShowTest();
    return;
  }
  drawResultScreen(success);
}

}
