#include "protocol.h"

#include <Arduino.h>
#include <cstdlib>
#include <cmath>
#include <cstring>
#include <cfloat>
#include "esp_random.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "../core/config.h"
#include "../crypto/ecdsa_exp.h"
#include "../experiment/experiment.h"
#include "../ina219/ina219.h"
#include "../benchmark/benchmark.h"
#include "../selftest/selftest.h"
#include "../batch/batch.h"
#include "../button/button.h"
#include "../rgbled/rgbled.h"
#include "../oled/oled.h"
#include "../test_vectors/test_vectors.h"

namespace rosef {

namespace {

char line[MAX_SERIAL_LINE];
size_t line_length = 0;
bool discarding_overlong_line = false;

 
 
 
bool g_command_failed = false;

void commandFailureLine(const __FlashStringHelper *message) {
  g_command_failed = true;
  Serial.println(message);
}

void commandFailurePrefix(const __FlashStringHelper *message) {
  g_command_failed = true;
  Serial.print(message);
}

void finishCommandRgbStatus() {
  if (g_command_failed) {
    rgbLedSetFail();
  } else {
    rgbLedSetPass();
  }
  
  oledShowCommandResult(!g_command_failed);
}

 
 
 
 
 
 
 
 
 
 
 
 

constexpr uint8_t TEST_PINS[] = {
  4,
  5,
  13,
  14,
  16,
  17
};

constexpr size_t TEST_PIN_COUNT =
    sizeof(TEST_PINS) / sizeof(TEST_PINS[0]);

void handleTestPR() {
  Serial.println(F("TESTPR START"));

  for (size_t i = 0; i < TEST_PIN_COUNT; ++i) {
    const uint8_t pin = TEST_PINS[i];

    pinMode(pin, OUTPUT);

     
    digitalWrite(pin, LOW);
    delay(100);

     
    Serial.print(F("TEST GPIO="));
    Serial.print(pin);
    Serial.println(F(" HIGH"));

    digitalWrite(pin, HIGH);
    delay(1000);

     
    digitalWrite(pin, LOW);

    Serial.print(F("TEST GPIO="));
    Serial.print(pin);
    Serial.println(F(" LOW"));

    delay(500);
  }

   
   
  pinMode(TRIGGER_GPIO, OUTPUT);
  digitalWrite(TRIGGER_GPIO, TRIGGER_IDLE_LEVEL);
  buttonBegin();

  Serial.println(F("TESTPR END (GPIO4=IDLE_HIGH GPIO27=INPUT_PULLUP)"));
}

constexpr uint32_t MAX_PULSE_US = 1000000;

void printIntro() {
   
   
   
  Serial.println();
  Serial.println(F("+==========================================================================+"));
  Serial.println(F("|                    ROSEF ESP32 - COMMAND CONSOLE                         |"));
  Serial.println(F("|             ECDSA TEST  |  BATCHES  |  INA219  |  GPIO4 TRIGGER           |"));
  Serial.println(F("+==========================================================================+"));
  Serial.println(F("SERIAL: 115200 baud | Set line ending to Both NL & CR in Arduino IDE."));
  Serial.println(F("Commands: keyword case-insensitive; SIGN message text keeps its case."));
  Serial.println();

  Serial.println(F("[0] MENU / HELP"));
  Serial.println(F("  HELP  |  MENU  |  ?                 Show this complete command list"));
  Serial.println();

  Serial.println(F("[1] CONNECTION / STATUS / DIAGNOSTICS"));
  Serial.println(F("  PING                                Check serial connection; reply OK PONG"));
  Serial.println(F("  INFO                                Show GPIO, serial, INA219, OLED and RGB LED info"));
  Serial.println(F("  STATUS                              Show uptime, free heap, CPU, GPIO4, button"));
  Serial.println(F("  FINAL                               Capture ONE test-key ECDSA operation + INA219 samples"));
  Serial.println(F("  SELFTEST                            Run ECDSA/self-test checks (no trigger pulses)"));
  Serial.println(F("  TESTPR                              Toggle diagnostic GPIOs; restores GPIO4 and button"));
  Serial.println(F("  BUTTON                              Read GPIO27 button state"));
  Serial.println(F("  BUTTON_EVENT                        Read and clear latched button-press event"));
  Serial.println(F("  OLEDTEST                            Verify OLED and show its I2C pins/address"));
  Serial.println(F("  RGB LED colors: green=ready/pass, orange=running, red=error"));
  Serial.println();

  Serial.println(F("[2] INA219 SENSOR"));
  Serial.println(F("  INA219                              Read one voltage/current/power sample"));
  Serial.println(F("  BATCH INA219 <count>                Read 1..1000 INA219 samples to Serial Monitor"));
  Serial.println(F("  Sensor wiring: SDA=GPIO21, SCL=GPIO22, default I2C address=0x40."));
  Serial.println(F("  OLED: SDA=GPIO18, SCK/SCL=GPIO19, I2C address auto-detect 0x3C/0x3D."));
  Serial.println(F("  Single-wire RGB LED (WS2812/NeoPixel compatible): data=GPIO25."));
  Serial.println();

  Serial.println(F("[3] ECDSA SIGNING / TEST PARAMETERS (TEST KEY ONLY)"));
  Serial.println(F("  SIGN <message>                      SHA-256 message, then sign and verify"));
  Serial.println(F("  SIGNHEX <64 hex digest chars>       Sign a 32-byte SHA-256 digest"));
  Serial.println(F("  SIGNK <64-hex nonce> <message>      Sign message with supplied test nonce"));
  Serial.println(F("  SIGNHEXK <nonce> <64-hex digest>    Sign digest with supplied test nonce"));
  Serial.println(F("  SETK <64-hex nonce>                 Set default test nonce in RAM"));
  Serial.println(F("  SETD <64-hex private scalar>        Set test private scalar in RAM"));
  Serial.println(F("  Note: hex scalars must be valid secp256k1 values; changes are RAM-only."));
  Serial.println();

  Serial.println(F("[4] ECDSA BATCH / PERFORMANCE"));
  Serial.println(F("  BATCH <count>                       Sign + verify 1..1000 test messages"));
  Serial.println(F("  SBATCH [count]                      GPIO4-only pulse train for PulseView; default 10, LOW/HIGH 250 ms"));
  Serial.println(F("  BENCH <count>                       Benchmark signing time, 1..1000 operations"));
  Serial.println(F("  BATCH uses the message 'rosef-batch' and increments the test nonce."));
  Serial.println(F("  Each BATCH item prints K/R/S, elapsed time, signature verification and trigger read-back."));
  Serial.println();

  Serial.println(F("[5] GPIO4 TRIGGER / CAPTURE EXPERIMENTS"));
  Serial.println(F("  PULSE [microseconds]                One active-LOW pulse, 1..1000000; default 1000"));
  Serial.println(F("  TRIGTEST [count]                    1..12 LOW/HIGH cycles; default 8 (~250 ms each level)"));
  Serial.println(F("  FULLIMP                             160 random-nonce test signatures + INA219 diagnostics"));
  Serial.println(F("  SCOPECAP                            160 random-nonce test signatures for external capture"));
  Serial.println(F("  GPIO4 idle=HIGH, active=LOW. Arm PulseView before PULSE/TRIGTEST/SCOPECAP."));
  Serial.println(F("  FULLIMP uses slow INA219 I2C readouts; it is not a high-speed current waveform."));
  Serial.println();

  Serial.println(F("[6] QUICK EXAMPLES - COPY A LINE AND PRESS ENTER"));
  Serial.println(F("  PING"));
  Serial.println(F("  INFO"));
  Serial.println(F("  OLEDTEST"));
  Serial.println(F("  STATUS"));
  Serial.println(F("  FINAL"));
  Serial.println(F("  INA219"));
  Serial.println(F("  BATCH 35"));
  Serial.println(F("  SBATCH 10"));
  Serial.println(F("  BENCH 100"));
  Serial.println(F("  BATCH INA219 20"));
  Serial.println(F("  SIGN Hello RoSEF"));
  Serial.println(F("  SIGNHEX <paste-64-hex-character-SHA256-digest>"));
  Serial.println(F("  PULSE 250000"));
  Serial.println(F("  TRIGTEST 8"));
  Serial.println(F("  FULLIMP"));
  Serial.println(F("  SCOPECAP"));
  Serial.println(F("  HELP"));
  Serial.println();
  Serial.println(F("IMPORTANT: TEST FIRMWARE ONLY. Never use SETD/SETK or nonce labels with production keys."));
  Serial.println(F("GPIO read-back PASS confirms ESP32 pad level only; it does not prove PulseView saw the wire."));
  Serial.println(F("+==========================================================================+"));
  Serial.println();
}
void printInfo() {
  Serial.print(F("OK ROSEF_ECDSA_EXP"));
  Serial.print(F(" SECP256K1"));
  Serial.print(F(" TRIGGER_GPIO="));
  Serial.print(TRIGGER_GPIO);
  Serial.print(F(" BUTTON_GPIO="));
  Serial.print(BUTTON_GPIO);
  Serial.print(F(" SERIAL="));
  Serial.println(SERIAL_BAUD);

  Serial.print(F(" INA219_SDA="));
  Serial.print(INA219_SDA_GPIO);

  Serial.print(F(" INA219_SCL="));
  Serial.print(INA219_SCL_GPIO);

  Serial.print(F(" INA219_ADDR=0x"));
  const uint8_t detected_address = ina219Address();
  if (detected_address == 0) {
    Serial.print(F("NONE"));
  } else {
    Serial.print(static_cast<unsigned int>(detected_address), HEX);
  }
  Serial.println();

  Serial.print(F(" OLED_SDA="));
  Serial.print(OLED_SDA_GPIO);
  Serial.print(F(" OLED_SCL="));
  Serial.print(OLED_SCL_GPIO);
  Serial.print(F(" OLED_ADDR="));
  if (!oledAvailable() || oledAddress() == 0) {
    Serial.print(F("NONE"));
  } else {
    Serial.print(F("0x"));
    Serial.print(static_cast<unsigned int>(oledAddress()), HEX);
  }
  Serial.print(F(" RGB_LED_GPIO="));
  Serial.println(RGB_LED_GPIO);
}

void printStatus() {
  Serial.print(F("OK STATUS"));

  Serial.print(F(" UPTIME_MS="));
  Serial.print(millis());

  Serial.print(F(" FREE_HEAP="));
  Serial.print(ESP.getFreeHeap());

  Serial.print(F(" CPU_MHZ="));
  Serial.print(ESP.getCpuFreqMHz());

  Serial.print(F(" TRIGGER="));
  Serial.print(
      digitalRead(TRIGGER_GPIO)
          ? F("HIGH")
          : F("LOW"));

  Serial.print(F(" BUTTON="));
  Serial.println(
      buttonIsPressed()
          ? F("PRESSED")
          : F("RELEASED"));
}

bool parseUnsigned(
    const char *text,
    uint32_t &value_out,
    uint32_t maximum) {

  if (text == nullptr) {
    return false;
  }

  while (*text == ' ' || *text == '\t') {
    ++text;
  }
  if (*text == '\0') {
    return false;
  }

  char *end = nullptr;
  const unsigned long parsed = strtoul(text, &end, 10);
  if (end == text) {
    return false;
  }

  while (*end == ' ' || *end == '\t') {
    ++end;
  }
  if (*end != '\0' || parsed == 0 || parsed > maximum) {
    return false;
  }

  value_out = static_cast<uint32_t>(parsed);
  return true;
}

bool parseNonce(
    const char *hex,
    uint8_t nonce[32]) {

  if (hex == nullptr || nonce == nullptr) {
    return false;
  }

  return hexToBytes(
      hex,
      nonce,
      32);
}

bool scalarIsInSecp256k1Range(const uint8_t scalar[32]) {
  static const uint8_t order[32] = {
      0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFE,
      0xBA, 0xAE, 0xDC, 0xE6, 0xAF, 0x48, 0xA0, 0x3B,
      0xBF, 0xD2, 0x5E, 0x8C, 0xD0, 0x36, 0x41, 0x41};
  uint8_t nonzero = 0;
  for (size_t i = 0; i < 32; ++i) nonzero |= scalar[i];
  return nonzero != 0 && memcmp(scalar, order, 32) < 0;
}

void printHex(
    const uint8_t *data,
    size_t length) {

  char buffer[65];

  if (data == nullptr || length > 32) {
    return;
  }

  bytesToHex(
      data,
      length,
      buffer,
      sizeof(buffer));

  Serial.print(buffer);
}

void printSignResult(
    const SignResult &result) {

  Serial.print(F("OK R="));
  printHex(
      result.r,
      sizeof(result.r));

  Serial.print(F(" S="));
  printHex(
      result.s,
      sizeof(result.s));

  Serial.print(F(" DT_US="));
  Serial.print(result.elapsed_us);
  if (result.trigger_enabled &&
      (!result.trigger_low_readback_ok || !result.trigger_high_readback_ok)) {
    g_command_failed = true;
  }
  if (!result.trigger_enabled) {
    Serial.println(F(" TRIGGER=SUPPRESSED"));
  } else {
    Serial.print(F(" TRIGGER_LOW_READBACK="));
    Serial.print(result.trigger_low_readback_ok ? F("PASS") : F("FAIL"));
    Serial.print(F(" TRIGGER_HIGH_READBACK="));
    Serial.println(result.trigger_high_readback_ok ? F("PASS") : F("FAIL"));
  }
}

void signWithDigest(const uint8_t digest[32], const uint8_t nonce[32]) {
  SignResult result{};
  if (!experimentSignDigest(digest, nonce, result)) {
    commandFailureLine(F("ERR SIGN_FAILED"));
    return;
  }
  if (!experimentVerifyDigest(digest, result)) {
    commandFailureLine(F("ERR SIGN_VERIFY_FAILED"));
    return;
  }
  printSignResult(result);
}

void handleSignMessage(const char *message, const uint8_t nonce[32]) {
  if (message == nullptr || *message == '\0') {
    commandFailureLine(F("ERR EMPTY_MESSAGE"));
    return;
  }
  uint8_t digest[32];
  if (!experimentSha256(message, digest)) {
    commandFailureLine(F("ERR SHA256_FAILED"));
    return;
  }
  SignResult result{};
  if (!experimentSignDigest(digest, nonce, result)) {
    commandFailureLine(F("ERR SIGN_FAILED"));
    return;
  }
  if (!experimentVerifyDigest(digest, result)) {
    commandFailureLine(F("ERR SIGN_VERIFY_FAILED"));
    return;
  }
  printSignResult(result);
}

void handleSetK(const char *hex) {
  uint8_t candidate[32];
  if (!hexToBytes(hex, candidate, sizeof(candidate)) ||
      !scalarIsInSecp256k1Range(candidate)) {
    commandFailureLine(F("ERR BAD_K (requires exactly 64 hex chars and 1 <= K < curve order)"));
    return;
  }
  memcpy(experimentDefaultNonce(), candidate, sizeof(candidate));
  Serial.println(F("OK K_SET"));
}

void handleSetD(const char *hex) {
  uint8_t candidate[32];
  if (!hexToBytes(hex, candidate, sizeof(candidate)) ||
      !scalarIsInSecp256k1Range(candidate)) {
    commandFailureLine(F("ERR BAD_D (requires exactly 64 hex chars and 1 <= D < curve order)"));
    return;
  }
  memcpy(experimentPrivateKey(), candidate, sizeof(candidate));
  Serial.println(F("OK D_SET"));
}

void handleSignHex(
    const char *hex) {

  uint8_t digest[32];

  if (!hexToBytes(
          hex,
          digest,
          sizeof(digest))) {
    commandFailureLine(F("ERR BAD_DIGEST"));
    return;
  }

  signWithDigest(
      digest,
      experimentDefaultNonce());
}

void handleSign(
    const char *message) {

  handleSignMessage(
      message,
      experimentDefaultNonce());
}

void handleSignK(
    char *payload) {

  if (payload == nullptr) {
    commandFailureLine(F("ERR BAD_SIGNK"));
    return;
  }

  if (strlen(payload) < 66) {
    commandFailureLine(F("ERR BAD_SIGNK"));
    return;
  }

  if (payload[64] != ' ') {
    commandFailureLine(F("ERR BAD_SIGNK"));
    return;
  }

  char k_hex[65];

  memcpy(
      k_hex,
      payload,
      64);

  k_hex[64] = '\0';

  uint8_t nonce[32];

  if (!parseNonce(k_hex, nonce) || !scalarIsInSecp256k1Range(nonce)) {
    commandFailureLine(F("ERR BAD_K"));
    return;
  }

  const char *message =
      payload + 65;

  handleSignMessage(
      message,
      nonce);
}

void handleSignHexK(
    char *payload) {

  if (payload == nullptr) {
    commandFailureLine(F("ERR BAD_SIGNHEXK"));
    return;
  }

  if (strlen(payload) != 129) {
    commandFailureLine(F("ERR BAD_SIGNHEXK"));
    return;
  }

  if (payload[64] != ' ') {
    commandFailureLine(F("ERR BAD_SIGNHEXK"));
    return;
  }

  char k_hex[65];
  char digest_hex[65];

  memcpy(
      k_hex,
      payload,
      64);

  k_hex[64] = '\0';

  memcpy(
      digest_hex,
      payload + 65,
      64);

  digest_hex[64] = '\0';

  uint8_t nonce[32];
  uint8_t digest[32];

  if (!parseNonce(k_hex, nonce) || !scalarIsInSecp256k1Range(nonce)) {
    commandFailureLine(F("ERR BAD_K"));
    return;
  }

  if (!hexToBytes(
          digest_hex,
          digest,
          sizeof(digest))) {
    commandFailureLine(F("ERR BAD_DIGEST"));
    return;
  }

  signWithDigest(
      digest,
      nonce);
}

void handlePulse(
    const char *argument) {

  uint32_t width_us =
      DEFAULT_TRIGGER_PULSE_US;

  if (argument != nullptr &&
      *argument != '\0') {

    if (!parseUnsigned(
            argument,
            width_us,
            MAX_PULSE_US)) {
      commandFailureLine(F("ERR BAD_PULSE"));
      return;
    }
  }

   
   
   
   
  pinMode(TRIGGER_GPIO, OUTPUT);
  digitalWrite(TRIGGER_GPIO, TRIGGER_IDLE_LEVEL);
  delayMicroseconds(5);

  const uint32_t start_us = micros();
  digitalWrite(TRIGGER_GPIO, TRIGGER_ACTIVE_LEVEL);
  delayMicroseconds(width_us);
  digitalWrite(TRIGGER_GPIO, TRIGGER_IDLE_LEVEL);

  const uint32_t elapsed_us = micros() - start_us;
  const int final_level = digitalRead(TRIGGER_GPIO);
  if (final_level != TRIGGER_IDLE_LEVEL) g_command_failed = true;

  Serial.print(F("OK PULSE WIDTH_US="));
  Serial.print(elapsed_us);
  Serial.print(F(" GPIO="));
  Serial.print(TRIGGER_GPIO);
  Serial.print(F(" FINAL_LEVEL="));
  Serial.println(final_level == HIGH ? F("HIGH") : F("LOW"));
}

constexpr uint32_t DEFAULT_TRIGGER_TEST_PULSES = 8;
constexpr uint32_t MAX_TRIGGER_TEST_PULSES = 12;

 
 
 
 
void handleTriggerTest(const char *argument) {
  uint32_t count = DEFAULT_TRIGGER_TEST_PULSES;
  if (argument != nullptr && *argument != '\0' &&
      !parseUnsigned(argument, count, MAX_TRIGGER_TEST_PULSES)) {
    commandFailureLine(F("ERR BAD_TRIGTEST_COUNT (use 1..12)"));
    return;
  }

  pinMode(TRIGGER_GPIO, OUTPUT);
  digitalWrite(TRIGGER_GPIO, TRIGGER_IDLE_LEVEL);
  delay(200);

  Serial.print(F("BEGIN TRIGTEST GPIO="));
  Serial.print(TRIGGER_GPIO);
  Serial.print(F(" COUNT="));
  Serial.println(count);
  Serial.println(F("TRIGTEST_NOTE=LOW 250ms, HIGH 250ms; arm PulseView before command"));

  bool readback_ok = true;
  for (uint32_t i = 0; i < count; ++i) {
    digitalWrite(TRIGGER_GPIO, TRIGGER_ACTIVE_LEVEL);
    delay(250);
    const int low_level = digitalRead(TRIGGER_GPIO);

    digitalWrite(TRIGGER_GPIO, TRIGGER_IDLE_LEVEL);
    const int high_level = digitalRead(TRIGGER_GPIO);
    delay(250);

    const bool this_ok = (low_level == LOW) && (high_level == HIGH);
    readback_ok = readback_ok && this_ok;
    Serial.print(F("TRIGTEST PULSE="));
    Serial.print(i + 1);
    Serial.print(F(" LOW_READBACK="));
    Serial.print(low_level == LOW ? F("PASS") : F("FAIL"));
    Serial.print(F(" HIGH_READBACK="));
    Serial.println(high_level == HIGH ? F("PASS") : F("FAIL"));
  }

  digitalWrite(TRIGGER_GPIO, TRIGGER_IDLE_LEVEL);
  if (!readback_ok) g_command_failed = true;
  Serial.println(readback_ok ? F("END TRIGTEST=PASS") : F("END TRIGTEST=FAIL"));
}

void handleBenchmark(
    const char *argument) {

  uint32_t count = 0;

  if (!parseUnsigned(
          argument,
          count,
          MAX_BENCH_COUNT)) {
    commandFailureLine(F("ERR BAD_BENCH_COUNT"));
    return;
  }

  BenchmarkResult result;

  if (!benchmarkRun(
          count,
          result)) {
    commandFailureLine(F("ERR BENCH_FAILED"));
    return;
  }

  Serial.print(F("OK BENCH"));

  Serial.print(F(" N="));
  Serial.print(result.count);

  Serial.print(F(" TOTAL_US="));
  Serial.print(
      static_cast<unsigned long>(
          result.total_us));

  Serial.print(F(" AVG_US="));
  Serial.print(result.average_us);

  Serial.print(F(" MIN_US="));
  Serial.print(result.min_us);

  Serial.print(F(" MAX_US="));
  Serial.println(result.max_us);
}

void handleBatch(
    const char *argument) {

  uint32_t count = 0;

  if (!parseUnsigned(
          argument,
          count,
          MAX_BATCH_COUNT)) {
    commandFailureLine(F("ERR BAD_BATCH_COUNT"));
    return;
  }

  BatchConfig config;

  config.sample_count = count;
  config.message = "rosef-batch";

  if (!batchRun(config)) {
    commandFailureLine(F("ERR BATCH_FAILED"));
    return;
  }

  Serial.println(F("OK BATCH=PASS"));
}

 
 
 
void handleSbatch(const char *argument) {
  constexpr uint32_t DEFAULT_SBATCH_COUNT = 10;
  constexpr uint32_t SBATCH_LOW_MS = 250;
  constexpr uint32_t SBATCH_HIGH_MS = 250;

  uint32_t count = DEFAULT_SBATCH_COUNT;
  const char *cursor = argument;
  while (cursor != nullptr && (*cursor == ' ' || *cursor == '\t')) {
    ++cursor;
  }
  if (cursor != nullptr && *cursor != '\0') {
    if (!parseUnsigned(cursor, count, MAX_BATCH_COUNT)) {
      commandFailureLine(F("ERR BAD_SBATCH_COUNT (use SBATCH [count], 1..1000)"));
      return;
    }
  }

  pinMode(TRIGGER_GPIO, OUTPUT);
  digitalWrite(TRIGGER_GPIO, TRIGGER_IDLE_LEVEL);
  delay(20);

  Serial.print(F("BEGIN SBATCH GPIO="));
  Serial.print(TRIGGER_GPIO);
  Serial.print(F(" COUNT="));
  Serial.print(count);
  Serial.println(F(" LOW_MS=250 HIGH_MS=250 MODE=GPIO_ONLY"));

  uint32_t readback_pass_count = 0;
  for (uint32_t i = 0; i < count; ++i) {
    digitalWrite(TRIGGER_GPIO, TRIGGER_ACTIVE_LEVEL);
    const bool low_ok = digitalRead(TRIGGER_GPIO) == TRIGGER_ACTIVE_LEVEL;
    delay(SBATCH_LOW_MS);

    digitalWrite(TRIGGER_GPIO, TRIGGER_IDLE_LEVEL);
    const bool high_ok = digitalRead(TRIGGER_GPIO) == TRIGGER_IDLE_LEVEL;
    if (low_ok && high_ok) {
      ++readback_pass_count;
    }
    delay(SBATCH_HIGH_MS);
  }

   
  digitalWrite(TRIGGER_GPIO, TRIGGER_IDLE_LEVEL);
  Serial.print(F("END SBATCH PULSES="));
  Serial.print(count);
  Serial.print(F(" READBACK_PASS="));
  Serial.print(readback_pass_count);
  Serial.print('/');
  Serial.println(count);
  if (readback_pass_count == count) {
    Serial.println(F("OK SBATCH=PASS"));
  } else {
    commandFailureLine(F("ERR SBATCH_GPIO_READBACK"));
  }
}

void handleFinal();   

void handleIna219() {
  if (!ina219Available() && !ina219FindOnce()) {
    commandFailureLine(F("ERR INA219_UNAVAILABLE"));
    return;
  }

  const Ina219Reading reading = ina219Read();
  if (!reading.valid) {
    commandFailureLine(F("ERR INA219_UNAVAILABLE"));
    return;
  }

  Serial.print(F("OK INA219 ADDR=0x"));
  const uint8_t address = ina219Address();
  if (address < 16) {
    Serial.print('0');
  }
  Serial.print(address, HEX);
  Serial.print(F(" V="));
  Serial.print(reading.bus_voltage_v, 6);
  Serial.print(F(" SHUNT_MV="));
  Serial.print(reading.shunt_voltage_mv, 6);
  Serial.print(F(" I_MA="));
  Serial.print(reading.current_ma, 6);
  Serial.print(F(" P_MW="));
  Serial.print(reading.power_mw, 6);
  Serial.print(F(" P_CALC_MW="));
  Serial.println(reading.calculated_power_mw, 6);
}

void handleBatchIna219(const char *argument) {
  uint32_t count = 0;
  if (!parseUnsigned(argument, count, MAX_BATCH_COUNT)) {
    commandFailureLine(F("ERR BAD_BATCH_COUNT"));
    return;
  }

   
   
  if (!ina219Available() && !ina219FindOnce()) {
    commandFailureLine(F("ERR INA219_UNAVAILABLE"));
    return;
  }

  Serial.println(F("BEGIN BATCH INA219"));
  Serial.print(F("META COUNT="));
  Serial.println(count);

  for (uint32_t i = 0; i < count; ++i) {
    const Ina219Reading reading = ina219Read();
    if (!reading.valid) {
      commandFailurePrefix(F("ERR INA219_READ ID="));
      Serial.println(i);
      Serial.println(F("END BATCH INA219"));
      return;
    }

    Serial.print(F("SAMPLE ID="));
    Serial.print(i);
    Serial.print(F(" V="));
    Serial.print(reading.bus_voltage_v, 6);
    Serial.print(F(" SHUNT_MV="));
    Serial.print(reading.shunt_voltage_mv, 6);
    Serial.print(F(" I_MA="));
    Serial.print(reading.current_ma, 6);
    Serial.print(F(" P_MW="));
    Serial.print(reading.power_mw, 6);
    Serial.print(F(" P_CALC_MW="));
    Serial.println(reading.calculated_power_mw, 6);
    delay(1);
  }

  Serial.println(F("END BATCH INA219"));
  Serial.println(F("OK BATCH INA219=PASS"));
}

 
 
 
 
 
 
constexpr uint32_t FULLIMP_BATCH_COUNT = 160;
bool g_fullimp_last_pass = false;
constexpr float FULLIMP_NOMINAL_LOAD_OHMS = 330.0f;
constexpr uint32_t FULLIMP_MAX_SAMPLES = 2048;
constexpr uint32_t FULLIMP_IDLE_TARGET = 20;
constexpr uint32_t FULLIMP_SAMPLE_PERIOD_MS = 20;
constexpr uint8_t FULLIMP_CAPTURE_NONE = 0;
constexpr uint8_t FULLIMP_CAPTURE_IDLE = 1;
constexpr uint8_t FULLIMP_CAPTURE_BATCH = 2;
constexpr uint8_t FULLIMP_PHASE_IDLE = 1;
constexpr uint8_t FULLIMP_PHASE_ECDSA = 2;
constexpr uint8_t FULLIMP_PHASE_BATCH_UNCLASSIFIED = 3;

struct FullImpSample {
   
   
   
  uint32_t read_begin_us;       
  uint32_t read_end_us;         
  float voltage_v;
  float current_ma;
  float power_mw;
  uint16_t operation;           
  uint8_t phase;
};

static inline uint32_t fullImpSampleMidpointUs(const FullImpSample &sample) {
  return sample.read_begin_us +
      (sample.read_end_us - sample.read_begin_us) / 2U;
}

struct FullImpOpStats {
  uint32_t sign_us;
  uint32_t trigger_start_rel_us;
  uint32_t trigger_end_rel_us;
  uint16_t sample_count;
  bool sign_ok;
  bool verify_ok;
  bool trigger_low_readback_ok;
  bool trigger_high_readback_ok;
  uint8_t nonce[32];   
  uint8_t r[32];
  uint8_t s[32];
  double current_sum;
  double voltage_sum;
  double power_sum;
  float peak_current_ma;
};

struct FullImpPhaseStats {
  uint32_t count = 0;
  double current_sum = 0.0;
  double voltage_sum = 0.0;
  double power_sum = 0.0;
  float min_current_ma = FLT_MAX;
  float max_current_ma = -FLT_MAX;
};

static FullImpSample g_fullimp_samples[FULLIMP_MAX_SAMPLES];
static float g_fullimp_sorted_values[FULLIMP_MAX_SAMPLES];
static FullImpOpStats g_fullimp_op_stats[FULLIMP_BATCH_COUNT];
static volatile uint32_t g_fullimp_sample_count = 0;
static volatile uint32_t g_fullimp_idle_sample_count = 0;
static volatile uint32_t g_fullimp_sample_read_errors = 0;
static volatile uint32_t g_fullimp_sample_overflows = 0;
static volatile uint16_t g_fullimp_current_operation = 0;
static volatile uint8_t g_fullimp_capture_mode = FULLIMP_CAPTURE_NONE;
static volatile bool g_fullimp_sampler_running = false;
static volatile bool g_fullimp_sampler_stop = false;
static uint32_t g_fullimp_capture_start_us = 0;

void fullImpSamplerTask(void *  ) {
  g_fullimp_sampler_running = true;
  TickType_t sample_delay = pdMS_TO_TICKS(FULLIMP_SAMPLE_PERIOD_MS);
  if (sample_delay == 0) sample_delay = 1;

  TickType_t last_wake_time = xTaskGetTickCount();
  while (!g_fullimp_sampler_stop) {
    const uint8_t capture_mode = g_fullimp_capture_mode;
    if (capture_mode != FULLIMP_CAPTURE_NONE) {
      const uint16_t operation = capture_mode == FULLIMP_CAPTURE_BATCH
          ? g_fullimp_current_operation : 0;
       
       
       
       
      const uint32_t read_begin_us = micros() - g_fullimp_capture_start_us;
      const Ina219Reading reading = ina219Read();
      const uint32_t read_end_us = micros() - g_fullimp_capture_start_us;

      uint8_t phase = 0;
      if (capture_mode == FULLIMP_CAPTURE_IDLE) {
        phase = FULLIMP_PHASE_IDLE;
      } else if (capture_mode == FULLIMP_CAPTURE_BATCH) {
        phase = FULLIMP_PHASE_BATCH_UNCLASSIFIED;
      }

      if (!reading.valid) {
        ++g_fullimp_sample_read_errors;
      } else if (phase != 0) {
        const uint32_t index = g_fullimp_sample_count;
        if (index < FULLIMP_MAX_SAMPLES) {
          FullImpSample &sample = g_fullimp_samples[index];
          sample.read_begin_us = read_begin_us;
          sample.read_end_us = read_end_us;
          sample.voltage_v = reading.bus_voltage_v;
          sample.current_ma = reading.current_ma;
          sample.power_mw = reading.power_mw;
          sample.operation = operation;
          sample.phase = phase;
          g_fullimp_sample_count = index + 1;
          if (phase == FULLIMP_PHASE_IDLE) {
            ++g_fullimp_idle_sample_count;
          }
        } else {
          ++g_fullimp_sample_overflows;
        }
      }
    }

    vTaskDelayUntil(&last_wake_time, sample_delay);
  }

   
  g_fullimp_sampler_running = false;
  vTaskDelete(nullptr);
}

bool stopFullImpSampler() {
  g_fullimp_capture_mode = FULLIMP_CAPTURE_NONE;
  g_fullimp_sampler_stop = true;
  const uint32_t wait_start = millis();
  while (g_fullimp_sampler_running &&
         static_cast<uint32_t>(millis() - wait_start) < 3000) {
    delay(1);
  }
  return !g_fullimp_sampler_running;
}

bool isValidSecp256k1Nonce(const uint8_t nonce[32]) {
  static const uint8_t order[32] = {
      0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
      0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFE,
      0xBA, 0xAE, 0xDC, 0xE6, 0xAF, 0x48, 0xA0, 0x3B,
      0xBF, 0xD2, 0x5E, 0x8C, 0xD0, 0x36, 0x41, 0x41};
  uint8_t nonzero = 0;
  for (size_t i = 0; i < 32; ++i) nonzero |= nonce[i];
  return nonzero != 0 && memcmp(nonce, order, 32) < 0;
}

void generateRandomTestNonce(uint8_t nonce[32], uint32_t previous_count) {
   
   
  bool duplicate = false;
  do {
    esp_fill_random(nonce, 32);
    duplicate = false;
    if (isValidSecp256k1Nonce(nonce)) {
      for (uint32_t previous = 0; previous < previous_count; ++previous) {
        if (memcmp(nonce, g_fullimp_op_stats[previous].nonce, 32) == 0) {
          duplicate = true;
          break;
        }
      }
    }
  } while (!isValidSecp256k1Nonce(nonce) || duplicate);
}

 
 
 
 
 
constexpr uint32_t FINAL_BASELINE_SAMPLES = 5;
constexpr uint32_t FINAL_SAMPLE_TIMEOUT_MS = 2000;

void handleFinal() {
  Serial.println();
  Serial.println(F("BEGIN FINAL ONE_SHOT REPORT V2"));
  Serial.println(F("MODE=ONE_SIGNING_OPERATION_TEST_KEY_ONLY"));
  Serial.println(F("BATCH_MODE=DISABLED OPERATION_COUNT=1"));

   
   
  const bool test_key_ok =
      memcmp(experimentPrivateKey(), TEST_PRIVATE_KEY, sizeof(TEST_PRIVATE_KEY)) == 0;
  if (!test_key_ok) g_command_failed = true;
  Serial.println(test_key_ok
      ? F("CHECK TEST_KEY=PASS SOURCE=COMPILED_TEST_VECTOR")
      : F("CHECK TEST_KEY=FAIL FINAL_IS_TEST_VECTOR_ONLY"));
  if (!test_key_ok) {
    Serial.println(F("KEY_RECOVERY=NOT_RUN_NON_TEST_KEY_ACTIVE"));
    commandFailureLine(F("FINAL_RESULT=REFUSED_NON_TEST_KEY"));
    Serial.println(F("END FINAL ONE_SHOT REPORT"));
    return;
  }

  const bool ina_ok = ina219Available() || ina219FindOnce();
  if (!ina_ok) {
    Serial.println(F("CHECK INA219=FAIL REASON=NOT_DETECTED"));
    commandFailureLine(F("FINAL_RESULT=FAIL_NO_SENSOR"));
    Serial.println(F("END FINAL ONE_SHOT REPORT"));
    return;
  }
   
   
   
  for (uint8_t warmup = 0; warmup < 2; ++warmup) {
    const Ina219Reading discard = ina219Read();
    if (!discard.valid) {
      Serial.println(F("CHECK INA219=FAIL REASON=WARMUP_READ_ERROR"));
      commandFailureLine(F("FINAL_RESULT=FAIL_SENSOR_WARMUP"));
      Serial.println(F("END FINAL ONE_SHOT REPORT"));
      return;
    }
    delay(5);
  }

  constexpr uint8_t FINAL_BASELINE_AVERAGE_COUNT = 5;
  double baseline_v_sum = 0.0;
  double baseline_shunt_sum = 0.0;
  double baseline_i_sum = 0.0;
  double baseline_p_reg_sum = 0.0;
  double baseline_p_calc_sum = 0.0;
  double baseline_i_sq_sum = 0.0;
  float baseline_i_min = FLT_MAX;
  float baseline_i_max = -FLT_MAX;
  for (uint8_t i = 0; i < FINAL_BASELINE_AVERAGE_COUNT; ++i) {
    const Ina219Reading baseline = ina219Read();
    if (!baseline.valid) {
      Serial.println(F("CHECK INA219=FAIL REASON=BASELINE_READ_ERROR"));
      commandFailureLine(F("FINAL_RESULT=FAIL_SENSOR_BASELINE"));
      Serial.println(F("END FINAL ONE_SHOT REPORT"));
      return;
    }
    baseline_v_sum += baseline.bus_voltage_v;
    baseline_shunt_sum += baseline.shunt_voltage_mv;
    baseline_i_sum += baseline.current_ma;
    baseline_p_reg_sum += baseline.power_mw;
    baseline_p_calc_sum += baseline.calculated_power_mw;
    baseline_i_sq_sum += static_cast<double>(baseline.current_ma) * baseline.current_ma;
    if (baseline.current_ma < baseline_i_min) baseline_i_min = baseline.current_ma;
    if (baseline.current_ma > baseline_i_max) baseline_i_max = baseline.current_ma;
    delay(5);
  }
  const double baseline_v_avg = baseline_v_sum / FINAL_BASELINE_AVERAGE_COUNT;
  const double baseline_shunt_avg = baseline_shunt_sum / FINAL_BASELINE_AVERAGE_COUNT;
  const double baseline_i_avg = baseline_i_sum / FINAL_BASELINE_AVERAGE_COUNT;
  const double baseline_p_reg_avg = baseline_p_reg_sum / FINAL_BASELINE_AVERAGE_COUNT;
  const double baseline_p_calc_avg = baseline_p_calc_sum / FINAL_BASELINE_AVERAGE_COUNT;
  const double baseline_i_variance = fmax(0.0,
      baseline_i_sq_sum / FINAL_BASELINE_AVERAGE_COUNT - baseline_i_avg * baseline_i_avg);
  const double baseline_i_std = sqrt(baseline_i_variance);

  Serial.print(F("CHECK INA219=PASS ADDR=0x"));
  if (ina219Address() < 16) Serial.print('0');
  Serial.print(ina219Address(), HEX);
  Serial.print(F(" CALIBRATION=32V_1A CURRENT_LSB_MA=0.040 POWER_LSB_MW=0.8"));
  Serial.print(F(" BASELINE_N=")); Serial.print(FINAL_BASELINE_AVERAGE_COUNT);
  Serial.print(F(" BASELINE_V=")); Serial.print(baseline_v_avg, 6);
  Serial.print(F(" BASELINE_SHUNT_MV=")); Serial.print(baseline_shunt_avg, 6);
  Serial.print(F(" BASELINE_I_MA=")); Serial.print(baseline_i_avg, 6);
  Serial.print(F(" BASELINE_I_MIN_MA=")); Serial.print(baseline_i_min, 6);
  Serial.print(F(" BASELINE_I_MAX_MA=")); Serial.print(baseline_i_max, 6);
  Serial.print(F(" BASELINE_I_STD_MA=")); Serial.print(baseline_i_std, 6);
  Serial.print(F(" BASELINE_P_MW=")); Serial.print(baseline_p_reg_avg, 6);
  Serial.print(F(" BASELINE_P_CALC_MW=")); Serial.println(baseline_p_calc_avg, 6);

  uint8_t digest[32];
  if (!experimentSha256("ROSEF-FINAL-ONE-SHOT-TEST", digest)) {
    Serial.println(F("CHECK SHA256=FAIL"));
    commandFailureLine(F("FINAL_RESULT=FAIL_DIGEST"));
    Serial.println(F("END FINAL ONE_SHOT REPORT"));
    return;
  }

   
   
  uint8_t nonce[32];
  generateRandomTestNonce(nonce, 0);
  memset(g_fullimp_samples, 0, sizeof(g_fullimp_samples));
  g_fullimp_sample_count = 0;
  g_fullimp_idle_sample_count = 0;
  g_fullimp_sample_read_errors = 0;
  g_fullimp_sample_overflows = 0;
  g_fullimp_current_operation = 1;
  g_fullimp_capture_mode = FULLIMP_CAPTURE_NONE;
  g_fullimp_sampler_stop = false;
  g_fullimp_sampler_running = false;
  g_fullimp_capture_start_us = micros();

   
   
  experimentSetTriggerEnabled(false);
  pinMode(TRIGGER_GPIO, OUTPUT);
  digitalWrite(TRIGGER_GPIO, TRIGGER_IDLE_LEVEL);

  TaskHandle_t sampler_task = nullptr;
  const BaseType_t sampler_created = xTaskCreatePinnedToCore(
      fullImpSamplerTask,
      "final_oneshot_sampler",
      4096,
      nullptr,
      2,
      &sampler_task,
      0);
  if (sampler_created != pdPASS) {
    experimentSetTriggerEnabled(true);
    Serial.println(F("CHECK INA219_SAMPLER=FAIL TASK_CREATE"));
    commandFailureLine(F("FINAL_RESULT=FAIL_SAMPLER_CREATE"));
    Serial.println(F("END FINAL ONE_SHOT REPORT"));
    return;
  }

  const uint32_t sampler_wait_start = millis();
  while (!g_fullimp_sampler_running &&
         static_cast<uint32_t>(millis() - sampler_wait_start) < 1000) {
    delay(1);
  }
  if (!g_fullimp_sampler_running) {
    g_fullimp_sampler_stop = true;
    experimentSetTriggerEnabled(true);
    Serial.println(F("CHECK INA219_SAMPLER=FAIL START_TIMEOUT"));
    commandFailureLine(F("FINAL_RESULT=FAIL_SAMPLER_START"));
    Serial.println(F("END FINAL ONE_SHOT REPORT"));
    return;
  }

   
  g_fullimp_capture_mode = FULLIMP_CAPTURE_IDLE;
  const uint32_t baseline_wait_start = millis();
  while (g_fullimp_idle_sample_count < FINAL_BASELINE_SAMPLES &&
         static_cast<uint32_t>(millis() - baseline_wait_start) < FINAL_SAMPLE_TIMEOUT_MS) {
    delay(1);
  }
  if (g_fullimp_idle_sample_count < FINAL_BASELINE_SAMPLES) {
    g_fullimp_capture_mode = FULLIMP_CAPTURE_NONE;
    const bool stopped = stopFullImpSampler();
    experimentSetTriggerEnabled(true);
    Serial.println(F("CHECK INA219_SAMPLER=FAIL BASELINE_TIMEOUT"));
    Serial.print(F("SAMPLES_OBTAINED=")); Serial.println(g_fullimp_sample_count);
    if (!stopped) g_command_failed = true;
    commandFailureLine(stopped
        ? F("FINAL_RESULT=FAIL_BASELINE_CAPTURE")
        : F("FINAL_RESULT=FAIL_SAMPLER_STOP"));
    Serial.println(F("END FINAL ONE_SHOT REPORT"));
    return;
  }

  g_fullimp_capture_mode = FULLIMP_CAPTURE_BATCH;
  const uint32_t trigger_start_abs_before = micros();
  digitalWrite(TRIGGER_GPIO, TRIGGER_ACTIVE_LEVEL);
  const uint32_t trigger_start_abs = trigger_start_abs_before;
  const bool low_readback_ok = digitalRead(TRIGGER_GPIO) == TRIGGER_ACTIVE_LEVEL;

  SignResult sign_result{};
  const bool sign_ok = experimentSignDigest(digest, nonce, sign_result);

   
  digitalWrite(TRIGGER_GPIO, TRIGGER_IDLE_LEVEL);
  const uint32_t trigger_end_abs = micros();
  const bool high_readback_ok = digitalRead(TRIGGER_GPIO) == TRIGGER_IDLE_LEVEL;
  g_fullimp_capture_mode = FULLIMP_CAPTURE_IDLE;

  const uint32_t trigger_start_rel = trigger_start_abs - g_fullimp_capture_start_us;
  const uint32_t trigger_end_rel = trigger_end_abs - g_fullimp_capture_start_us;
  const uint32_t sign_window_us = trigger_end_abs - trigger_start_abs_before;

   
   
  const uint32_t post_target = g_fullimp_sample_count + FINAL_BASELINE_SAMPLES;
  const uint32_t post_wait_start = millis();
  while (g_fullimp_sample_count < post_target &&
         static_cast<uint32_t>(millis() - post_wait_start) < FINAL_SAMPLE_TIMEOUT_MS) {
    delay(1);
  }
  const bool post_samples_ok = g_fullimp_sample_count >= post_target;
  g_fullimp_capture_mode = FULLIMP_CAPTURE_NONE;
  const bool sampler_stopped = stopFullImpSampler();
  experimentSetTriggerEnabled(true);

  const bool verify_ok = sign_ok && experimentVerifyDigest(digest, sign_result);
  const uint32_t sample_count = g_fullimp_sample_count;
  uint32_t pre_count = 0, overlap_count = 0, post_count = 0, fully_inside_count = 0;
  double pre_current_sum = 0.0;
  double fully_inside_current_sum = 0.0;
  uint32_t sample_interval_min_us = 0xFFFFFFFFUL;
  uint32_t sample_interval_max_us = 0;
  double sample_interval_sum_us = 0.0;
  uint32_t sample_interval_count = 0;
  uint32_t read_duration_min_us = 0xFFFFFFFFUL;
  uint32_t read_duration_max_us = 0;
  double read_duration_sum_us = 0.0;
  uint32_t read_duration_count = 0;
  for (uint32_t i = 0; i < sample_count; ++i) {
    const FullImpSample &sample = g_fullimp_samples[i];
    const bool overlaps = sample.read_begin_us <= trigger_end_rel &&
                          sample.read_end_us >= trigger_start_rel;
    const uint32_t read_duration_us = sample.read_end_us - sample.read_begin_us;
    if (read_duration_us < read_duration_min_us) read_duration_min_us = read_duration_us;
    if (read_duration_us > read_duration_max_us) read_duration_max_us = read_duration_us;
    read_duration_sum_us += read_duration_us;
    ++read_duration_count;
    if (i > 0) {
      const uint32_t interval_us = fullImpSampleMidpointUs(sample) - fullImpSampleMidpointUs(g_fullimp_samples[i - 1]);
      if (interval_us < sample_interval_min_us) sample_interval_min_us = interval_us;
      if (interval_us > sample_interval_max_us) sample_interval_max_us = interval_us;
      sample_interval_sum_us += interval_us;
      ++sample_interval_count;
    }
    if (overlaps) {
      ++overlap_count;
      if (sample.read_begin_us >= trigger_start_rel &&
          sample.read_end_us <= trigger_end_rel) {
        ++fully_inside_count;
        fully_inside_current_sum += sample.current_ma;
      }
    } else if (sample.read_end_us < trigger_start_rel) {
      ++pre_count;
      pre_current_sum += sample.current_ma;
    } else if (sample.read_begin_us > trigger_end_rel) {
      ++post_count;
    }
  }

  Serial.println(F("SECTION SINGLE_OPERATION"));
  Serial.print(F("CHECK ECDSA_SIGN=")); Serial.println(sign_ok ? F("PASS") : F("FAIL"));
  Serial.print(F("CHECK ECDSA_VERIFY=")); Serial.println(verify_ok ? F("PASS") : F("FAIL"));
  Serial.print(F("CHECK GPIO4_LOW_READBACK=")); Serial.println(low_readback_ok ? F("PASS") : F("FAIL"));
  Serial.print(F("CHECK GPIO4_HIGH_READBACK=")); Serial.println(high_readback_ok ? F("PASS") : F("FAIL"));
  Serial.print(F("TRIGGER_START_REL_US=")); Serial.println(trigger_start_rel);
  Serial.print(F("TRIGGER_END_REL_US=")); Serial.println(trigger_end_rel);
  Serial.print(F("TRIGGER_WINDOW_US=")); Serial.println(trigger_end_rel - trigger_start_rel);
  Serial.print(F("SIGN_CALL_WINDOW_US=")); Serial.println(sign_window_us);
  Serial.print(F("NONCE_EXPORTED=")); Serial.println(F("NO"));
  if (sign_ok) {
    Serial.print(F("OPMETA Z=")); bytesToHex(digest, sizeof(digest), line, sizeof(line)); Serial.print(line);
    Serial.print(F(" R=")); bytesToHex(sign_result.r, sizeof(sign_result.r), line, sizeof(line)); Serial.print(line);
    Serial.print(F(" S=")); bytesToHex(sign_result.s, sizeof(sign_result.s), line, sizeof(line)); Serial.print(line);
    Serial.println();
  }

  Serial.println(F("SECTION INA219_TRACE"));
  Serial.print(F("SAMPLE_PERIOD_TARGET_MS=")); Serial.println(FULLIMP_SAMPLE_PERIOD_MS);
  Serial.print(F("SAMPLE_INTERVAL_MIN_US=")); Serial.println(sample_interval_count ? sample_interval_min_us : 0);
  Serial.print(F("SAMPLE_INTERVAL_MAX_US=")); Serial.println(sample_interval_count ? sample_interval_max_us : 0);
  Serial.print(F("SAMPLE_INTERVAL_AVG_US=")); Serial.println(
      sample_interval_count ? sample_interval_sum_us / sample_interval_count : 0.0, 2);
  Serial.print(F("I2C_READ_DURATION_MIN_US=")); Serial.println(read_duration_count ? read_duration_min_us : 0);
  Serial.print(F("I2C_READ_DURATION_MAX_US=")); Serial.println(read_duration_count ? read_duration_max_us : 0);
  Serial.print(F("I2C_READ_DURATION_AVG_US=")); Serial.println(
      read_duration_count ? read_duration_sum_us / read_duration_count : 0.0, 2);
  Serial.println(F("POWER_NOTE=P_MW is INA219 register (0.8mW/LSB after 32V_1A calibration); P_CALC_MW=BUS_V*CURRENT_MA"));
  Serial.print(F("TRACE_COUNTS TOTAL=")); Serial.print(sample_count);
  Serial.print(F(" PRE=")); Serial.print(pre_count);
  Serial.print(F(" OVERLAP_TRIGGER=")); Serial.print(overlap_count);
  Serial.print(F(" FULLY_INSIDE_TRIGGER=")); Serial.print(fully_inside_count);
  Serial.print(F(" POST=")); Serial.print(post_count);
  Serial.print(F(" READ_ERRORS=")); Serial.print(g_fullimp_sample_read_errors);
  Serial.print(F(" OVERFLOWS=")); Serial.println(g_fullimp_sample_overflows);
  for (uint32_t i = 0; i < sample_count; ++i) {
    const FullImpSample &sample = g_fullimp_samples[i];
    const bool overlaps = sample.read_begin_us <= trigger_end_rel &&
                          sample.read_end_us >= trigger_start_rel;
    const char *phase = overlaps ? "OVERLAP" :
        (sample.read_end_us < trigger_start_rel ? "PRE" : "POST");
    Serial.print(F("TRACE ID=")); Serial.print(i);
    Serial.print(F(" T_REL_US=")); Serial.print(fullImpSampleMidpointUs(sample));
    Serial.print(F(" READ_BEGIN_US=")); Serial.print(sample.read_begin_us);
    Serial.print(F(" READ_END_US=")); Serial.print(sample.read_end_us);
    Serial.print(F(" PHASE=")); Serial.print(phase);
    Serial.print(F(" V=")); Serial.print(sample.voltage_v, 6);
    Serial.print(F(" I_MA=")); Serial.print(sample.current_ma, 6);
    Serial.print(F(" P_MW=")); Serial.print(sample.power_mw, 6);
    Serial.print(F(" P_CALC_MW=")); Serial.println(sample.voltage_v * sample.current_ma, 6);
  }
  if (pre_count > 0 && fully_inside_count > 0) {
    Serial.print(F("INA219_MEAN_DELTA_I_MA="));
    Serial.println(fully_inside_current_sum / fully_inside_count - pre_current_sum / pre_count, 6);
    Serial.println(F("INA219_DELTA_METHOD=FULLY_INSIDE_TRIGGER_READS_MINUS_PRE_READS"));
  } else {
    Serial.println(F("INA219_MEAN_DELTA_I_MA=UNAVAILABLE_INSUFFICIENT_PRE_OR_FULLY_INSIDE_SAMPLES"));
  }
  Serial.println(F("NOTE INA219_IS_A_LOW_RATE_I2C_SENSOR_NOT_A_HIGH_SPEED_CURRENT_WAVEFORM_ADC"));
  Serial.println(F("KEY_RECOVERY=NOT_DEMONSTRATED_BY_THIS_SINGLE_CAPTURE"));
  Serial.println(F("KEY_RECOVERY_REASON=INA219_TRACE_RATE_AND_SINGLE_TRACE_DO_NOT_SUPPORT_RELIABLE_ECDSA_NONCE_RECOVERY"));
  Serial.print(F("CHECK INA219_OVERLAP_SAMPLES=")); Serial.println(overlap_count > 0 ? F("PASS") : F("FAIL_NO_OVERLAP"));
  Serial.print(F("CHECK POST_CAPTURE_SAMPLES=")); Serial.println(post_samples_ok ? F("PASS") : F("FAIL_TIMEOUT"));
  Serial.print(F("CHECK SAMPLER_STOP=")); Serial.println(sampler_stopped ? F("PASS") : F("FAIL"));
  const bool capture_ok = sign_ok && verify_ok && low_readback_ok && high_readback_ok &&
      sampler_stopped && post_samples_ok && overlap_count > 0;
  if (!capture_ok) g_command_failed = true;
  if (capture_ok) {
    Serial.println(F("FINAL_RESULT=ONE_SHOT_TRACE_CAPTURED_KEY_NOT_RECOVERED"));
  } else {
    commandFailureLine(F("FINAL_RESULT=FAIL_ONE_SHOT_CAPTURE_OR_NO_SAMPLE_OVERLAP"));
  }
  Serial.println(F("END FINAL ONE_SHOT REPORT"));
}

void sortFloatValues(float *values, size_t count) {
  for (size_t i = 1; i < count; ++i) {
    const float key = values[i];
    size_t j = i;
    while (j > 0 && values[j - 1] > key) {
      values[j] = values[j - 1];
      --j;
    }
    values[j] = key;
  }
}

float medianOfSorted(const float *values, uint32_t count) {
  if (count == 0) return 0.0f;
  if (count % 2 != 0) return values[count / 2];
  return (values[count / 2 - 1] + values[count / 2]) / 2.0f;
}

void handleFullImpImpl() {
  g_fullimp_last_pass = false;
  bool all_ok = true;
  Serial.println(F("BEGIN FULLIMP V3"));
  Serial.println(F("MODE=TEST_VECTORS_ONLY NONCE_POLICY=RANDOM_TEST_NONCES_GROUND_TRUTH_SIDECAR"));

   
   
  const bool test_vectors_ok =
      memcmp(experimentPrivateKey(), TEST_PRIVATE_KEY, 32) == 0 &&
      memcmp(experimentDefaultNonce(), TEST_DEFAULT_NONCE, 32) == 0;
  Serial.println(test_vectors_ok
      ? F("CHECK TEST_VECTORS=PASS")
      : F("CHECK TEST_VECTORS=FAIL (FULLIMP is test-only; restore test vectors first)"));
  if (!test_vectors_ok) {
    Serial.println(F("END FULLIMP"));
    commandFailureLine(F("ERR FULLIMP=TEST_VECTORS_REQUIRED"));
    return;
  }

   
   
  experimentSetTriggerEnabled(false);
  const bool crypto_selftest_ok = selfTestRun();
  Serial.println(crypto_selftest_ok
      ? F("CHECK SELFTEST=PASS")
      : F("CHECK SELFTEST=FAIL"));
  all_ok = all_ok && crypto_selftest_ok;

  bool ina_ok = ina219Available() || ina219FindOnce();
  Ina219Reading initial_reading{};
  if (ina_ok) {
    initial_reading = ina219Read();
    ina_ok = initial_reading.valid;
  }
  if (ina_ok) {
    Serial.print(F("CHECK INA219=PASS ADDR=0x"));
    const uint8_t address = ina219Address();
    if (address < 16) Serial.print('0');
    Serial.print(address, HEX);
    Serial.print(F(" V="));
    Serial.print(initial_reading.bus_voltage_v, 6);
    Serial.print(F(" I_MA="));
    Serial.println(initial_reading.current_ma, 6);
  } else {
    Serial.println(F("CHECK INA219=FAIL"));
  }
  all_ok = all_ok && ina_ok;

  pinMode(TRIGGER_GPIO, OUTPUT);
  digitalWrite(TRIGGER_GPIO, TRIGGER_IDLE_LEVEL);
  const bool trigger_idle_readback_ok =
      digitalRead(TRIGGER_GPIO) == TRIGGER_IDLE_LEVEL;
  Serial.print(F("CHECK TRIGGER_GPIO=")); Serial.print(TRIGGER_GPIO);
  Serial.print(F(" IDLE_HIGH_READBACK="));
  Serial.println(trigger_idle_readback_ok ? F("PASS") : F("FAIL"));
  all_ok = all_ok && trigger_idle_readback_ok;

  uint8_t verify_digest[32];
  SignResult verify_probe{};
  const bool sign_probe_ok =
      experimentSha256("hello", verify_digest) &&
      experimentSignDigest(verify_digest, experimentDefaultNonce(), verify_probe);
  const bool verify_path_ok = sign_probe_ok &&
      experimentVerifyDigest(verify_digest, verify_probe);
  Serial.println(sign_probe_ok
      ? F("CHECK ECDSA_SIGN=PASS")
      : F("CHECK ECDSA_SIGN=FAIL"));
  Serial.println(verify_path_ok
      ? F("CHECK ECDSA_VERIFY=PASS METHOD=MBEDTLS_VERIFY_WITH_DERIVED_PUBLIC_KEY")
      : F("CHECK ECDSA_VERIFY=FAIL"));
  all_ok = all_ok && sign_probe_ok && verify_path_ok;

  if (!crypto_selftest_ok || !ina_ok || !sign_probe_ok || !verify_path_ok) {
    experimentSetTriggerEnabled(true);
    Serial.println(F("BATCH SKIPPED: required pre-check failed"));
    Serial.println(F("END FULLIMP"));
    commandFailureLine(F("ERR FULLIMP=FAIL"));
    return;
  }

  uint8_t digest[32];
  if (!experimentSha256("FULLIMP-BATCH-160-V3", digest)) {
    experimentSetTriggerEnabled(true);
    commandFailureLine(F("ERR FULLIMP_DIGEST"));
    Serial.println(F("END FULLIMP"));
    commandFailureLine(F("ERR FULLIMP=FAIL"));
    return;
  }

  memset(g_fullimp_samples, 0, sizeof(g_fullimp_samples));
  memset(g_fullimp_op_stats, 0, sizeof(g_fullimp_op_stats));
  g_fullimp_sample_count = 0;
  g_fullimp_idle_sample_count = 0;
  g_fullimp_sample_read_errors = 0;
  g_fullimp_sample_overflows = 0;
  g_fullimp_current_operation = 0;
  g_fullimp_capture_mode = FULLIMP_CAPTURE_NONE;
  g_fullimp_sampler_stop = false;
  g_fullimp_sampler_running = false;
  g_fullimp_capture_start_us = micros();

  TaskHandle_t sampler_task = nullptr;
  const BaseType_t sampler_created = xTaskCreatePinnedToCore(
      fullImpSamplerTask,
      "fullimp_sampler",
      4096,
      nullptr,
      2,
      &sampler_task,
      0);
  if (sampler_created != pdPASS) {
    experimentSetTriggerEnabled(true);
    Serial.println(F("CHECK INA219_SAMPLER=FAIL TASK_CREATE"));
    Serial.println(F("END FULLIMP"));
    commandFailureLine(F("ERR FULLIMP=FAIL"));
    return;
  }

  const uint32_t sampler_start_wait = millis();
  while (!g_fullimp_sampler_running &&
         static_cast<uint32_t>(millis() - sampler_start_wait) < 1000) {
    delay(1);
  }
  if (!g_fullimp_sampler_running) {
    g_fullimp_sampler_stop = true;
    experimentSetTriggerEnabled(true);
    Serial.println(F("CHECK INA219_SAMPLER=FAIL TASK_START_TIMEOUT"));
    Serial.println(F("END FULLIMP"));
    commandFailureLine(F("ERR FULLIMP=FAIL"));
    return;
  }

   
  g_fullimp_capture_mode = FULLIMP_CAPTURE_IDLE;
  const uint32_t idle_wait_start = millis();
  while (g_fullimp_idle_sample_count < FULLIMP_IDLE_TARGET &&
         static_cast<uint32_t>(millis() - idle_wait_start) < 3000) {
    delay(1);
  }
  const bool idle_capture_ok =
      g_fullimp_idle_sample_count >= FULLIMP_IDLE_TARGET;
  Serial.print(F("CHECK IDLE_CAPTURE="));
  Serial.print(idle_capture_ok ? F("PASS") : F("FAIL"));
  Serial.print(F(" SAMPLES="));
  Serial.println(g_fullimp_idle_sample_count);
  all_ok = all_ok && idle_capture_ok;

  if (!idle_capture_ok) {
    const bool stopped = stopFullImpSampler();
    (void)stopped;
    experimentSetTriggerEnabled(true);
    Serial.println(F("END FULLIMP"));
    commandFailureLine(F("ERR FULLIMP=FAIL"));
    return;
  }

  uint8_t nonce[32];

  Serial.println(F("CHECK INA219_SAMPLER=PASS CORE=0 BUFFER=2048"));
  Serial.println(F("BEGIN FULLIMP BATCH=160 MODE=TEST_VECTOR_RANDOM_NONCES+TRIGGER+INA219"));
  Serial.print(F("SAMPLE_PERIOD_MS=")); Serial.print(FULLIMP_SAMPLE_PERIOD_MS); Serial.println(F(" (target start-to-start period; actual interval and I2C read durations are reported in trace diagnostics)"));
  Serial.println(F("INA219_NOTE=I2C READOUT SAMPLES ARE DIAGNOSTIC, NOT A HIGH-SPEED CURRENT WAVEFORM"));
  Serial.println(F("SCOPE_NOTE=USE GPIO4 ACTIVE-LOW AS THE EXTERNAL OSCILLOSCOPE TRIGGER"));
  Serial.println(F("THEORY MODEL: I_MA = BUS_VOLTAGE_V / 330_OHM * 1000; nominal-load estimate only"));
  Serial.println(F("INA219_SAMPLING=ENABLED_ONLY_DURING_ECDSA_SIGN; DISABLED_DURING_VERIFY"));
  g_fullimp_capture_mode = FULLIMP_CAPTURE_NONE;
  experimentSetTriggerEnabled(true);

  uint32_t completed = 0;
  uint32_t verify_pass_count = 0;
  for (uint32_t i = 0; i < FULLIMP_BATCH_COUNT; ++i) {
    g_fullimp_current_operation = static_cast<uint16_t>(i + 1);
    generateRandomTestNonce(nonce, i);
    FullImpOpStats &op_stats = g_fullimp_op_stats[i];
    memcpy(op_stats.nonce, nonce, sizeof(op_stats.nonce));

    SignResult sign_result{};
     
     
     
    g_fullimp_capture_mode = FULLIMP_CAPTURE_BATCH;
    const bool sign_ok = experimentSignDigest(digest, nonce, sign_result);
    g_fullimp_capture_mode = FULLIMP_CAPTURE_NONE;
    op_stats.sign_ok = sign_ok;
    op_stats.trigger_low_readback_ok = sign_result.trigger_enabled && sign_result.trigger_low_readback_ok;
    op_stats.trigger_high_readback_ok = sign_result.trigger_enabled && sign_result.trigger_high_readback_ok;
    op_stats.sign_us = sign_result.elapsed_us;
    op_stats.trigger_start_rel_us = sign_result.trigger_start_us - g_fullimp_capture_start_us;
    op_stats.trigger_end_rel_us = sign_result.trigger_end_us - g_fullimp_capture_start_us;
    if (!op_stats.trigger_low_readback_ok || !op_stats.trigger_high_readback_ok) {
      all_ok = false;
    }

    if (!sign_ok) {
      Serial.print(F("BATCH ")); Serial.print(i + 1);
      Serial.println(F(": ECDSA_SIGN=FAIL"));
      all_ok = false;
      break;
    }
    memcpy(op_stats.r, sign_result.r, sizeof(op_stats.r));
    memcpy(op_stats.s, sign_result.s, sizeof(op_stats.s));

    op_stats.verify_ok = experimentVerifyDigest(digest, sign_result);
    if (op_stats.verify_ok) {
      ++verify_pass_count;
    } else {
      all_ok = false;
    }
    ++completed;
    delay(1);
  }

  experimentSetTriggerEnabled(false);
  const bool sampler_stopped = stopFullImpSampler();
  experimentSetTriggerEnabled(true);
  if (!sampler_stopped) {
    Serial.println(F("CHECK INA219_SAMPLER=FAIL STOP_TIMEOUT"));
    all_ok = false;
  }

   
   
   
  for (uint32_t sample_index = 0; sample_index < g_fullimp_sample_count; ++sample_index) {
    FullImpSample &sample = g_fullimp_samples[sample_index];
    if (sample.phase != FULLIMP_PHASE_BATCH_UNCLASSIFIED) continue;
    for (uint32_t op_index = 0; op_index < completed; ++op_index) {
      const FullImpOpStats &op = g_fullimp_op_stats[op_index];
      if (fullImpSampleMidpointUs(sample) >= op.trigger_start_rel_us &&
          fullImpSampleMidpointUs(sample) <= op.trigger_end_rel_us) {
        sample.phase = FULLIMP_PHASE_ECDSA;
        sample.operation = static_cast<uint16_t>(op_index + 1);
        break;
      }
    }
    if (sample.phase == FULLIMP_PHASE_BATCH_UNCLASSIFIED) {
       
      sample.phase = 0;
      sample.operation = 0;
    }
  }

  FullImpPhaseStats idle_stats;
  FullImpPhaseStats active_stats;
  uint32_t active_sample_count = 0;
  uint32_t idle_sample_count = 0;
  for (uint32_t i = 0; i < g_fullimp_sample_count; ++i) {
    const FullImpSample &sample = g_fullimp_samples[i];
    FullImpPhaseStats *phase_stats = nullptr;
    if (sample.phase == FULLIMP_PHASE_IDLE) {
      phase_stats = &idle_stats;
      ++idle_sample_count;
    } else if (sample.phase == FULLIMP_PHASE_ECDSA) {
      phase_stats = &active_stats;
      ++active_sample_count;
      if (sample.operation > 0 && sample.operation <= FULLIMP_BATCH_COUNT) {
        FullImpOpStats &op = g_fullimp_op_stats[sample.operation - 1];
        ++op.sample_count;
        op.current_sum += sample.current_ma;
        op.voltage_sum += sample.voltage_v;
        op.power_sum += sample.power_mw;
        if (op.sample_count == 1 || sample.current_ma > op.peak_current_ma) {
          op.peak_current_ma = sample.current_ma;
        }
      }
      if (active_sample_count <= FULLIMP_MAX_SAMPLES) {
        g_fullimp_sorted_values[active_sample_count - 1] = sample.current_ma;
      }
    }

    if (phase_stats != nullptr) {
      ++phase_stats->count;
      phase_stats->current_sum += sample.current_ma;
      phase_stats->voltage_sum += sample.voltage_v;
      phase_stats->power_sum += sample.power_mw;
      if (sample.current_ma < phase_stats->min_current_ma) {
        phase_stats->min_current_ma = sample.current_ma;
      }
      if (sample.current_ma > phase_stats->max_current_ma) {
        phase_stats->max_current_ma = sample.current_ma;
      }
    }
  }

  for (uint32_t i = 0; i < FULLIMP_BATCH_COUNT; ++i) {
    if (i >= completed) break;
    const FullImpOpStats &op = g_fullimp_op_stats[i];
    if (!op.verify_ok) all_ok = false;
  }
  if (g_fullimp_sample_read_errors != 0 ||
      g_fullimp_sample_overflows != 0 ||
      completed != FULLIMP_BATCH_COUNT ||
      verify_pass_count != completed) {
    all_ok = false;
  }

   
  float median_active_current = 0.0f;
  if (active_sample_count > 0) {
    sortFloatValues(g_fullimp_sorted_values, active_sample_count);
    median_active_current = medianOfSorted(g_fullimp_sorted_values, active_sample_count);
  }

   
   
  uint32_t theory_count = 0;
  double theory_sum = 0.0;
  for (uint32_t i = 0; i < g_fullimp_sample_count; ++i) {
    const FullImpSample &sample = g_fullimp_samples[i];
    if (sample.phase != FULLIMP_PHASE_ECDSA) continue;
    const float theory_ma =
        (sample.voltage_v / FULLIMP_NOMINAL_LOAD_OHMS) * 1000.0f;
    if (theory_count < FULLIMP_MAX_SAMPLES) {
      g_fullimp_sorted_values[theory_count++] = theory_ma;
      theory_sum += theory_ma;
    }
  }
  float median_theory_current = 0.0f;
  if (theory_count > 0) {
    sortFloatValues(g_fullimp_sorted_values, theory_count);
    median_theory_current = medianOfSorted(g_fullimp_sorted_values, theory_count);
  }
  const double active_avg_current = active_stats.count
      ? active_stats.current_sum / active_stats.count : 0.0;
  const double idle_avg_current = idle_stats.count
      ? idle_stats.current_sum / idle_stats.count : 0.0;
  const double delta_current = active_avg_current - idle_avg_current;
  const double delta_current_percent = idle_avg_current > 0.0
      ? (delta_current / idle_avg_current) * 100.0 : 0.0;
  const double median_off_by_percent = median_theory_current > 0.0f
      ? (fabsf(median_active_current - median_theory_current) /
         median_theory_current) * 100.0 : 0.0;

  Serial.println(F("END FULLIMP BATCH"));
  Serial.println(F("BEGIN FULLIMP OPMETA TEST_VECTOR_ONLY"));
  for (uint32_t i = 0; i < completed; ++i) {
    const FullImpOpStats &op = g_fullimp_op_stats[i];
    Serial.print(F("OPMETA OP=")); Serial.print(i + 1);
    Serial.print(F(" Z=")); printHex(digest, sizeof(digest));
    Serial.print(F(" R=")); printHex(op.r, sizeof(op.r));
    Serial.print(F(" S=")); printHex(op.s, sizeof(op.s));
    Serial.print(F(" K_GT=")); printHex(op.nonce, sizeof(op.nonce));
    Serial.print(F(" TRIGGER_START_T_US=")); Serial.print(op.trigger_start_rel_us);
    Serial.print(F(" TRIGGER_END_T_US=")); Serial.print(op.trigger_end_rel_us);
    Serial.print(F(" SIGN_US=")); Serial.print(op.sign_us);
    Serial.print(F(" VERIFY=")); Serial.print(op.verify_ok ? F("PASS") : F("FAIL"));
    Serial.print(F(" TRIGGER_LOW_READBACK=")); Serial.print(op.trigger_low_readback_ok ? F("PASS") : F("FAIL"));
    Serial.print(F(" TRIGGER_HIGH_READBACK=")); Serial.println(op.trigger_high_readback_ok ? F("PASS") : F("FAIL"));
  }
  Serial.println(F("END FULLIMP OPMETA"));
  Serial.println(F("BEGIN FULLIMP OPERATION SUMMARY"));
  for (uint32_t i = 0; i < completed; ++i) {
    const FullImpOpStats &op = g_fullimp_op_stats[i];
    const double op_avg_current = op.sample_count
        ? op.current_sum / op.sample_count : 0.0;
    const double op_avg_voltage = op.sample_count
        ? op.voltage_sum / op.sample_count : 0.0;
    const double op_avg_power = op.sample_count
        ? op.power_sum / op.sample_count : 0.0;
     
     
    const double op_avg_calculated_power = op_avg_voltage * op_avg_current;

    Serial.print(F("BATCH ")); Serial.print(i + 1);
    Serial.print(F(": ECDSA_SIGN=")); Serial.print(op.sign_ok ? F("PASS") : F("FAIL"));
    Serial.print(F(" ECDSA_VERIFY=")); Serial.print(op.verify_ok ? F("PASS") : F("FAIL"));
    Serial.print(F(" SIGN_US=")); Serial.print(op.sign_us);
    Serial.print(F(" TRIGGER_LOW_READBACK=")); Serial.print(op.trigger_low_readback_ok ? F("PASS") : F("FAIL"));
    Serial.print(F(" TRIGGER_HIGH_READBACK=")); Serial.print(op.trigger_high_readback_ok ? F("PASS") : F("FAIL"));
    Serial.print(F(" TRIGGER_START_T_US=")); Serial.print(op.trigger_start_rel_us);
    Serial.print(F(" TRIGGER_END_T_US=")); Serial.print(op.trigger_end_rel_us);
    Serial.print(F(" CAPTURE_SAMPLES=")); Serial.print(op.sample_count);
    Serial.print(F(" INA219: AVG_V=")); Serial.print(op_avg_voltage, 6);
    Serial.print(F(" AVG_I_MA=")); Serial.print(op_avg_current, 6);
    Serial.print(F(" MAX_SAMPLED_I_MA=")); Serial.print(op.peak_current_ma, 6);
    Serial.print(F(" AVG_P_MW=")); Serial.print(op_avg_power, 6);
    Serial.print(F(" AVG_P_CALC_MW=")); Serial.println(op_avg_calculated_power, 6);
  }
  Serial.println(F("END FULLIMP OPERATION SUMMARY"));

  Serial.println(F("BEGIN FULLIMP IDLE SAMPLES"));
  for (uint32_t i = 0; i < g_fullimp_sample_count; ++i) {
    const FullImpSample &sample = g_fullimp_samples[i];
    if (sample.phase != FULLIMP_PHASE_IDLE) continue;
    Serial.print(F("IDLE_SAMPLE T_US=")); Serial.print(fullImpSampleMidpointUs(sample));
    Serial.print(F(" V=")); Serial.print(sample.voltage_v, 6);
    Serial.print(F(" I_MA=")); Serial.print(sample.current_ma, 6);
    Serial.print(F(" P_MW=")); Serial.println(sample.power_mw, 6);
  }
  Serial.println(F("END FULLIMP IDLE SAMPLES"));

  Serial.println(F("BEGIN FULLIMP ECDSA SAMPLES"));
  for (uint32_t i = 0; i < g_fullimp_sample_count; ++i) {
    const FullImpSample &sample = g_fullimp_samples[i];
    if (sample.phase != FULLIMP_PHASE_ECDSA) continue;
    Serial.print(F("SAMPLE OP=")); Serial.print(sample.operation);
    Serial.print(F(" TRIGGER=ACTIVE_LOW T_US=")); Serial.print(fullImpSampleMidpointUs(sample));
    Serial.print(F(" READ_BEGIN_T_US=")); Serial.print(sample.read_begin_us);
    Serial.print(F(" READ_END_T_US=")); Serial.print(sample.read_end_us);
    Serial.print(F(" V=")); Serial.print(sample.voltage_v, 6);
    Serial.print(F(" I_MA=")); Serial.print(sample.current_ma, 6);
    Serial.print(F(" P_MW=")); Serial.print(sample.power_mw, 6);
    Serial.print(F(" P_CALC_MW=")); Serial.println(sample.voltage_v * sample.current_ma, 6);
  }
  Serial.println(F("END FULLIMP ECDSA SAMPLES"));

  Serial.print(F("SUMMARY COUNT=")); Serial.print(completed);
  Serial.print(F(" VERIFY_PASS_COUNT=")); Serial.print(verify_pass_count);
  Serial.print(F(" IDLE_SAMPLES=")); Serial.print(idle_sample_count);
  Serial.print(F(" ECDSA_ACTIVE_SAMPLES=")); Serial.print(active_sample_count);
  Serial.print(F(" SAMPLER_READ_ERRORS=")); Serial.print(g_fullimp_sample_read_errors);
  Serial.print(F(" SAMPLE_BUFFER_OVERFLOWS=")); Serial.println(g_fullimp_sample_overflows);
  Serial.print(F("IDLE_AVG_INA_MA=")); Serial.println(idle_avg_current, 6);
  Serial.print(F("IDLE_MIN_INA_MA=")); Serial.println(
      idle_stats.count ? idle_stats.min_current_ma : 0.0f, 6);
  Serial.print(F("IDLE_MAX_INA_MA=")); Serial.println(
      idle_stats.count ? idle_stats.max_current_ma : 0.0f, 6);
  Serial.print(F("ECDSA_SAMPLE_AVG_INA_MA=")); Serial.println(active_avg_current, 6);
  Serial.print(F("MEDIAN_ECDSA_SAMPLE_INA_MA=")); Serial.println(median_active_current, 6);
  Serial.print(F("MAX_SAMPLED_ECDSA_INA_MA=")); Serial.println(
      active_stats.count ? active_stats.max_current_ma : 0.0f, 6);
  Serial.print(F("DELTA_CURRENT_MA=")); Serial.println(delta_current, 6);
  Serial.print(F("DELTA_CURRENT_PERCENT_OF_IDLE=")); Serial.println(delta_current_percent, 2);
  Serial.print(F("AVG_THEORY_MA_R330=")); Serial.println(
      theory_count ? theory_sum / theory_count : 0.0, 6);
  Serial.print(F("MEDIAN_THEORY_MA_R330=")); Serial.println(median_theory_current, 6);
  Serial.print(F("MEDIAN_INA_OFF_BY_PERCENT=")); Serial.println(median_off_by_percent, 2);
  double total_sign_us = 0.0;
  for (uint32_t i = 0; i < completed; ++i) {
    total_sign_us += g_fullimp_op_stats[i].sign_us;
  }
  Serial.print(F("AVG_ECDSA_SIGN_US="));
  Serial.println(completed ? total_sign_us / completed : 0.0, 2);
  Serial.println(F("NOTE: INA219 samples are slow I2C register readouts, not a high-speed instantaneous current waveform."));
  Serial.println(F("NOTE: 330-ohm theory is an estimate; physical Logic Analyzer capture must be validated in PulseView."));

  Serial.println(F("END FULLIMP"));
  g_fullimp_last_pass = all_ok && completed == FULLIMP_BATCH_COUNT &&
      idle_sample_count >= FULLIMP_IDLE_TARGET;
  Serial.println(g_fullimp_last_pass
      ? F("OK FULLIMP=PASS") : F("ERR FULLIMP=FAIL"));
}

 
 
void handleFullImp() {
  g_fullimp_last_pass = false;
  rgbLedSetRunning();
  handleFullImpImpl();
  if (g_fullimp_last_pass) {
    rgbLedSetPass();
  } else {
    g_command_failed = true;
    rgbLedSetFail();
  }
}

 
 
 
 
void handleScopeCapture() {
  bool all_ok = true;
  Serial.println(F("BEGIN SCOPECAP V1"));
  Serial.println(F("MODE=TEST_VECTOR_ONLY NONCE_POLICY=RANDOM_TEST_NONCES_GROUND_TRUTH_SIDECAR"));

  const bool test_vectors_ok =
      memcmp(experimentPrivateKey(), TEST_PRIVATE_KEY, 32) == 0 &&
      memcmp(experimentDefaultNonce(), TEST_DEFAULT_NONCE, 32) == 0;
  Serial.println(test_vectors_ok
      ? F("CHECK TEST_VECTORS=PASS")
      : F("CHECK TEST_VECTORS=FAIL (SCOPECAP is test-only; restore test vectors first)"));
  if (!test_vectors_ok) {
    Serial.println(F("END SCOPECAP"));
    commandFailureLine(F("ERR SCOPECAP=TEST_VECTORS_REQUIRED"));
    return;
  }

   
   
  experimentSetTriggerEnabled(false);
  const bool crypto_selftest_ok = selfTestRun();
  Serial.println(crypto_selftest_ok
      ? F("CHECK SELFTEST=PASS TRIGGER_SUPPRESSED=YES")
      : F("CHECK SELFTEST=FAIL"));
  all_ok = all_ok && crypto_selftest_ok;

  uint8_t verify_digest[32];
  SignResult verify_probe{};
  const bool sign_probe_ok =
      experimentSha256("hello", verify_digest) &&
      experimentSignDigest(verify_digest, experimentDefaultNonce(), verify_probe);
  const bool verify_path_ok = sign_probe_ok &&
      experimentVerifyDigest(verify_digest, verify_probe);
  Serial.println(sign_probe_ok
      ? F("CHECK ECDSA_SIGN=PASS TRIGGER_SUPPRESSED=YES")
      : F("CHECK ECDSA_SIGN=FAIL"));
  Serial.println(verify_path_ok
      ? F("CHECK ECDSA_VERIFY=PASS METHOD=MBEDTLS_VERIFY_WITH_DERIVED_PUBLIC_KEY")
      : F("CHECK ECDSA_VERIFY=FAIL"));
  all_ok = all_ok && sign_probe_ok && verify_path_ok;

  uint8_t digest[32];
  const bool digest_ok = experimentSha256("FULLIMP-BATCH-160-V3", digest);
  all_ok = all_ok && digest_ok;
  if (!crypto_selftest_ok || !sign_probe_ok || !verify_path_ok || !digest_ok) {
    experimentSetTriggerEnabled(true);
    Serial.println(F("BATCH SKIPPED: required pre-check failed"));
    Serial.println(F("END SCOPECAP"));
    commandFailureLine(F("ERR SCOPECAP=FAIL"));
    return;
  }

  memset(g_fullimp_op_stats, 0, sizeof(g_fullimp_op_stats));
  uint8_t nonce[32];
  uint32_t completed = 0;
  uint32_t verify_pass_count = 0;
  g_fullimp_capture_start_us = micros();
  pinMode(TRIGGER_GPIO, OUTPUT);
  digitalWrite(TRIGGER_GPIO, TRIGGER_IDLE_LEVEL);
  Serial.println(F("CHECK TRIGGER_GPIO=4 IDLE=HIGH ACTIVE=LOW"));
  Serial.println(F("BEGIN SCOPECAP BATCH=160 MODE=EXTERNAL_SCOPE_CURRENT_WAVEFORM"));
  Serial.println(F("CAPTURE_NOTE=ARM EXTERNAL SCOPE ON GPIO4 FALLING EDGE BEFORE THIS BATCH"));
  Serial.println(F("INA219_SAMPLER=DISABLED_TO_AVOID_I2C_READOUT_INTERFERENCE"));

  experimentSetTriggerEnabled(true);
  for (uint32_t i = 0; i < FULLIMP_BATCH_COUNT; ++i) {
    generateRandomTestNonce(nonce, i);
    FullImpOpStats &op = g_fullimp_op_stats[i];
    memcpy(op.nonce, nonce, sizeof(op.nonce));
    SignResult sign_result{};
    op.sign_ok = experimentSignDigest(digest, nonce, sign_result);
    op.trigger_low_readback_ok = sign_result.trigger_enabled && sign_result.trigger_low_readback_ok;
    op.trigger_high_readback_ok = sign_result.trigger_enabled && sign_result.trigger_high_readback_ok;
    op.sign_us = sign_result.elapsed_us;
    op.trigger_start_rel_us = sign_result.trigger_start_us - g_fullimp_capture_start_us;
    op.trigger_end_rel_us = sign_result.trigger_end_us - g_fullimp_capture_start_us;
    if (!op.trigger_low_readback_ok || !op.trigger_high_readback_ok) all_ok = false;
    if (!op.sign_ok) {
      all_ok = false;
      break;
    }
    memcpy(op.r, sign_result.r, sizeof(op.r));
    memcpy(op.s, sign_result.s, sizeof(op.s));
    op.verify_ok = experimentVerifyDigest(digest, sign_result);
    if (op.verify_ok) {
      ++verify_pass_count;
    } else {
      all_ok = false;
    }
    ++completed;
    delay(1);
  }
  experimentSetTriggerEnabled(false);
  experimentSetTriggerEnabled(true);   

  Serial.println(F("END SCOPECAP BATCH"));
  Serial.println(F("BEGIN SCOPECAP OPMETA TEST_VECTOR_ONLY"));
  for (uint32_t i = 0; i < completed; ++i) {
    const FullImpOpStats &op = g_fullimp_op_stats[i];
    Serial.print(F("OPMETA OP=")); Serial.print(i + 1);
    Serial.print(F(" Z=")); printHex(digest, sizeof(digest));
    Serial.print(F(" R=")); printHex(op.r, sizeof(op.r));
    Serial.print(F(" S=")); printHex(op.s, sizeof(op.s));
    Serial.print(F(" K_GT=")); printHex(op.nonce, sizeof(op.nonce));
    Serial.print(F(" TRIGGER_START_T_US=")); Serial.print(op.trigger_start_rel_us);
    Serial.print(F(" TRIGGER_END_T_US=")); Serial.print(op.trigger_end_rel_us);
    Serial.print(F(" SIGN_US=")); Serial.print(op.sign_us);
    Serial.print(F(" VERIFY=")); Serial.print(op.verify_ok ? F("PASS") : F("FAIL"));
    Serial.print(F(" TRIGGER_LOW_READBACK=")); Serial.print(op.trigger_low_readback_ok ? F("PASS") : F("FAIL"));
    Serial.print(F(" TRIGGER_HIGH_READBACK=")); Serial.println(op.trigger_high_readback_ok ? F("PASS") : F("FAIL"));
  }
  Serial.println(F("END SCOPECAP OPMETA"));
  Serial.print(F("SUMMARY COUNT=")); Serial.print(completed);
  Serial.print(F(" VERIFY_PASS_COUNT=")); Serial.println(verify_pass_count);
  Serial.println(F("END SCOPECAP"));
  const bool pass = all_ok && completed == FULLIMP_BATCH_COUNT && verify_pass_count == completed;
  if (!pass) g_command_failed = true;
  Serial.println(pass ? F("OK SCOPECAP=PASS") : F("ERR SCOPECAP=FAIL"));
}

void handleButton() {
  Serial.print(F("OK BUTTON="));

  Serial.println(
      buttonIsPressed()
          ? F("PRESSED")
          : F("RELEASED"));
}

void handleButtonEvent() {
  Serial.print(F("OK BUTTON_EVENT="));

  Serial.println(
      buttonWasPressed()
          ? F("PRESSED")
          : F("NONE"));
}

void dispatchLine(
    char *command) {

  if (command == nullptr) return;

   
   
  while (*command == ' ' || *command == '\t') ++command;
  if (*command == '\0') return;
  char *first_end = command;
  while (*first_end != '\0' && *first_end != ' ' && *first_end != '\t') {
    if (*first_end >= 'a' && *first_end <= 'z') *first_end -= ('a' - 'A');
    ++first_end;
  }
  if ((first_end - command) == 5 && memcmp(command, "BATCH", 5) == 0) {
    char *second = first_end;
    while (*second == ' ' || *second == '\t') ++second;
    char *second_end = second;
    while (*second_end != '\0' && *second_end != ' ' && *second_end != '\t') {
      if (*second_end >= 'a' && *second_end <= 'z') *second_end -= ('a' - 'A');
      ++second_end;
    }
  }

  if (strcmp(command, "?") == 0 || strcmp(command, "MENU") == 0) {
    printIntro();
    return;
  }

  if (strcmp(
          command,
          "PING") == 0) {

    Serial.println(F("OK PONG"));
    return;
  }

  if (strcmp(
          command,
          "INFO") == 0) {

    printInfo();
    return;
  }

  if (strcmp(
          command,
          "STATUS") == 0) {

    printStatus();
    return;
  }

  if (strcmp(command, "FINAL") == 0) {
    handleFinal();
    return;
  }

  if (strcmp(command, "SCOPECAP") == 0) {
    handleScopeCapture();
    return;
  }

  if (strcmp(command, "FULLIMP") == 0) {
    handleFullImp();
    return;
  }

  if (strcmp(command, "SELFTEST") == 0) {
    experimentSetTriggerEnabled(false);
    const bool passed = selfTestRun();
    experimentSetTriggerEnabled(true);
    if (!passed) g_command_failed = true;
    Serial.println(passed ? F("OK SELFTEST=PASS") : F("ERR SELFTEST=FAIL"));
    return;
  }

  if (strcmp(
          command,
          "HELP") == 0) {

    printIntro();
    return;
  }

   
   
   
  if (strcmp(
          command,
          "TESTPR") == 0) {

    handleTestPR();
    return;
  }

  if (strcmp(command, "OLEDTEST") == 0) {
    if (!oledAvailable() && !oledBegin()) {
      commandFailureLine(F("ERR OLED_UNAVAILABLE"));
      return;
    }
    oledShowTest();
    Serial.println(F("OK OLEDTEST=PASS"));
    return;
  }

  if (strcmp(
          command,
          "INA219") == 0) {

    handleIna219();
    return;
  }

  if (strcmp(
          command,
          "BUTTON") == 0) {

    handleButton();
    return;
  }

  if (strcmp(
          command,
          "BUTTON_EVENT") == 0) {

    handleButtonEvent();
    return;
  }

  if (strcmp(command, "TRIGTEST") == 0) {
    handleTriggerTest(nullptr);
    return;
  }

  if (strncmp(command, "TRIGTEST ", 9) == 0) {
    handleTriggerTest(command + 9);
    return;
  }

  if (strcmp(
          command,
          "PULSE") == 0) {

    handlePulse(nullptr);
    return;
  }

  if (strncmp(
          command,
          "PULSE ",
          6) == 0) {

    handlePulse(command + 6);
    return;
  }

  if (strncmp(
          command,
          "SETK ",
          5) == 0) {

    handleSetK(command + 5);
    return;
  }

  if (strncmp(
          command,
          "SETD ",
          5) == 0) {

    handleSetD(command + 5);
    return;
  }

  if (strncmp(
          command,
          "SIGNHEXK ",
          9) == 0) {

    handleSignHexK(
        command + 9);
    return;
  }

  if (strncmp(
          command,
          "SIGNK ",
          6) == 0) {

    handleSignK(
        command + 6);
    return;
  }

  if (strncmp(
          command,
          "SIGNHEX ",
          8) == 0) {

    handleSignHex(
        command + 8);
    return;
  }

  if (strncmp(
          command,
          "SIGN ",
          5) == 0) {

    handleSign(
        command + 5);
    return;
  }

  if (strncmp(
          command,
          "BENCH ",
          6) == 0) {

    handleBenchmark(
        command + 6);
    return;
  }

   
   
  if ((strncmp(command, "SBATCH", 6) == 0 &&
       (command[6] == '\0' || command[6] == ' ' || command[6] == '\t')) ||
      (strncmp(command, "SBTACH", 6) == 0 &&
       (command[6] == '\0' || command[6] == ' ' || command[6] == '\t'))) {
    handleSbatch(command + 6);
    return;
  }

   
   
  if (strcmp(command, "BATCH INA219") == 0) {
    handleBatchIna219("");
    return;
  }

  if (strncmp(command, "BATCH INA219 ", 13) == 0) {
    handleBatchIna219(command + 13);
    return;
  }

  if (strncmp(
          command,
          "BATCH ",
          6) == 0) {

    handleBatch(
        command + 6);
    return;
  }

  if (*command != '\0') {
    commandFailureLine(F("ERR UNKNOWN_COMMAND"));
    Serial.println(F("Type HELP, MENU or ? to list valid commands."));
  }
}

 
 
void handleLine(char *command) {
  if (command == nullptr) return;
  const char *cursor = command;
  while (*cursor == ' ' || *cursor == '\t') ++cursor;
  if (*cursor == '\0') return;

  g_command_failed = false;
  rgbLedSetCommandRunning();
  oledShowCommandRunning(command);
  dispatchLine(command);
  finishCommandRgbStatus();
}

}

void protocolBegin() {
  Serial.begin(SERIAL_BAUD);
  rgbLedBegin();
  delay(1000);

  const bool oled_ok = oledBegin();
  if (oled_ok) {
    oledShowBoot();
  } else {
    Serial.println(F("WARN OLED_NOT_DETECTED (check OLED_I2C diagnostics above)"));
  }

  experimentBegin();
  buttonBegin();

  printIntro();

  Serial.println(F("READY ROSEF_ECDSA_EXP"));
  printInfo();
}

void protocolLoop() {
  static bool button_was_down = false;
  const bool button_down = buttonIsPressed();

   
   
  if (button_down && !button_was_down) {
    g_command_failed = false;
    rgbLedSetCommandRunning();
    oledShowCommandRunning("BUTTON: FULLIMP");
    Serial.println(F("BUTTON_EVENT=FULLIMP_START"));
    handleFullImp();
    Serial.println(F("BUTTON_EVENT=FULLIMP_END"));
    finishCommandRgbStatus();
  }
  button_was_down = button_down;

  while (Serial.available() > 0) {
    const int value =
        Serial.read();

    if (value < 0) {
      return;
    }

    const char c =
        static_cast<char>(value);

    if (c == '\r' || c == '\n') {
      if (discarding_overlong_line) {
        discarding_overlong_line = false;
        line_length = 0;
        g_command_failed = false;
        rgbLedSetCommandRunning();
        oledShowCommandRunning("LINE_TOO_LONG");
        commandFailureLine(F("ERR LINE_TOO_LONG"));
        finishCommandRgbStatus();
        continue;
      }
      if (line_length == 0) continue;
      line[line_length] = '\0';
      handleLine(line);
      line_length = 0;
      continue;
    }

    if (discarding_overlong_line) continue;
    if (line_length + 1 >= sizeof(line)) {
       
       
      discarding_overlong_line = true;
      line_length = 0;
      continue;
    }
    line[line_length++] = c;
  }
}

}