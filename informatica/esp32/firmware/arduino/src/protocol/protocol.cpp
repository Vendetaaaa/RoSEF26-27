#include "protocol.h"

#include <Arduino.h>
#include <cstdlib>
#include <cstring>

#include "../core/config.h"
#include "../crypto/ecdsa_exp.h"
#include "../experiment/experiment.h"
#include "../ina219/ina219.h"
#include "../benchmark/benchmark.h"
#include "../selftest/selftest.h"
#include "../batch/batch.h"
#include "../button/button.h"

namespace rosef {

namespace {

char line[MAX_SERIAL_LINE];
size_t line_length = 0;

// ============================================================
// TEMPORARY LOGIC ANALYZER CONNECTIVITY TEST
// ============================================================
//
// These are GPIOs that can safely be used as temporary outputs.
// Each pin will be HIGH for 1 second and LOW for 0.5 seconds.
//
// Watch D0-D7 in PulseView.
// The channel that changes during "TEST GPIO=X HIGH"
// is the analyzer channel connected to GPIO X.
//
// ============================================================

constexpr uint8_t TEST_PINS[] = {
  4,
  5,
  13,
  14,
  16,
  17,
  18,
  19
};

constexpr size_t TEST_PIN_COUNT =
    sizeof(TEST_PINS) / sizeof(TEST_PINS[0]);

void handleTestPR() {
  Serial.println(F("TESTPR START"));

  for (size_t i = 0; i < TEST_PIN_COUNT; ++i) {
    const uint8_t pin = TEST_PINS[i];

    pinMode(pin, OUTPUT);

    // Make sure the pin starts LOW.
    digitalWrite(pin, LOW);
    delay(100);

    // HIGH for 1 second.
    Serial.print(F("TEST GPIO="));
    Serial.print(pin);
    Serial.println(F(" HIGH"));

    digitalWrite(pin, HIGH);
    delay(1000);

    // LOW for 0.5 second.
    digitalWrite(pin, LOW);

    Serial.print(F("TEST GPIO="));
    Serial.print(pin);
    Serial.println(F(" LOW"));

    delay(500);
  }

  // Restore the project's trigger pin to its normal state.
  pinMode(TRIGGER_GPIO, OUTPUT);
  digitalWrite(TRIGGER_GPIO, TRIGGER_IDLE_LEVEL);

  Serial.println(F("TESTPR END"));
}

constexpr uint32_t MAX_PULSE_US = 1000000;

void printIntro() {
  Serial.println(F("COMMANDS"));
  Serial.println(F("PING - testa conexiunea"));
  Serial.println(F("INFO - arata configuratia"));
  Serial.println(F("STATUS - arata starea"));
  Serial.println(F("SELFTEST - ruleaza autotestul"));
  Serial.println(F("SETK - seteaza nonce-ul"));
  Serial.println(F("SETD - seteaza cheia"));
  Serial.println(F("SIGN - semneaza mesajul"));
  Serial.println(F("SIGNHEX - semneaza digestul"));
  Serial.println(F("SIGNK - semneaza cu nonce"));
  Serial.println(F("SIGNHEXK - semneaza digest+nonce"));
  Serial.println(F("BENCH - ruleaza benchmark-ul"));
  Serial.println(F("BATCH - ruleaza testul batch"));
  Serial.println(F("PULSE - genereaza un puls"));
  Serial.println(F("TESTPR - testeaza GPIO-urile"));
  Serial.println(F("BUTTON - citeste butonul"));
  Serial.println(F("BUTTON_EVENT - citeste evenimentul"));
  Serial.println(F("INA219 - cauta si citeste senzorul"));
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

  if (text == nullptr || *text == '\0') {
    return false;
  }

  char *end = nullptr;

  const unsigned long parsed =
      strtoul(text, &end, 10);

  if (end == text || *end != '\0') {
    return false;
  }

  if (parsed == 0 || parsed > maximum) {
    return false;
  }

  value_out =
      static_cast<uint32_t>(parsed);

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
  Serial.println(result.elapsed_us);
}

void signWithDigest(
    const uint8_t digest[32],
    const uint8_t nonce[32]) {

  SignResult result;

  if (!experimentSignDigest(
          digest,
          nonce,
          result)) {
    Serial.println(F("ERR SIGN_FAILED"));
    return;
  }

  printSignResult(result);
}

void handleSignMessage(
    const char *message,
    const uint8_t nonce[32]) {

  if (message == nullptr || *message == '\0') {
    Serial.println(F("ERR EMPTY_MESSAGE"));
    return;
  }

  SignResult result;

  if (!experimentSignMessage(
          message,
          nonce,
          result)) {
    Serial.println(F("ERR SIGN_FAILED"));
    return;
  }

  printSignResult(result);
}

void handleSetK(
    const char *hex) {

  uint8_t *nonce =
      experimentDefaultNonce();

  if (!hexToBytes(
          hex,
          nonce,
          32)) {
    Serial.println(F("ERR BAD_K"));
    return;
  }

  Serial.println(F("OK K_SET"));
}

void handleSetD(
    const char *hex) {

  uint8_t *private_key =
      experimentPrivateKey();

  if (!hexToBytes(
          hex,
          private_key,
          32)) {
    Serial.println(F("ERR BAD_D"));
    return;
  }

  Serial.println(F("OK D_SET"));
}

void handleSignHex(
    const char *hex) {

  uint8_t digest[32];

  if (!hexToBytes(
          hex,
          digest,
          sizeof(digest))) {
    Serial.println(F("ERR BAD_DIGEST"));
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
    Serial.println(F("ERR BAD_SIGNK"));
    return;
  }

  if (strlen(payload) < 66) {
    Serial.println(F("ERR BAD_SIGNK"));
    return;
  }

  if (payload[64] != ' ') {
    Serial.println(F("ERR BAD_SIGNK"));
    return;
  }

  char k_hex[65];

  memcpy(
      k_hex,
      payload,
      64);

  k_hex[64] = '\0';

  uint8_t nonce[32];

  if (!parseNonce(
          k_hex,
          nonce)) {
    Serial.println(F("ERR BAD_K"));
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
    Serial.println(F("ERR BAD_SIGNHEXK"));
    return;
  }

  if (strlen(payload) != 129) {
    Serial.println(F("ERR BAD_SIGNHEXK"));
    return;
  }

  if (payload[64] != ' ') {
    Serial.println(F("ERR BAD_SIGNHEXK"));
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

  if (!parseNonce(
          k_hex,
          nonce)) {
    Serial.println(F("ERR BAD_K"));
    return;
  }

  if (!hexToBytes(
          digest_hex,
          digest,
          sizeof(digest))) {
    Serial.println(F("ERR BAD_DIGEST"));
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
      Serial.println(F("ERR BAD_PULSE"));
      return;
    }
  }

  const uint32_t start_us =
      micros();

  digitalWrite(
      TRIGGER_GPIO,
      HIGH);

  delayMicroseconds(width_us);

  digitalWrite(
      TRIGGER_GPIO,
      LOW);

  const uint32_t elapsed_us =
      micros() - start_us;

  Serial.print(F("OK PULSE WIDTH_US="));
  Serial.println(elapsed_us);
}

void handleBenchmark(
    const char *argument) {

  uint32_t count = 0;

  if (!parseUnsigned(
          argument,
          count,
          MAX_BENCH_COUNT)) {
    Serial.println(F("ERR BAD_BENCH_COUNT"));
    return;
  }

  BenchmarkResult result;

  if (!benchmarkRun(
          count,
          result)) {
    Serial.println(F("ERR BENCH_FAILED"));
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
    Serial.println(F("ERR BAD_BATCH_COUNT"));
    return;
  }

  BatchConfig config;

  config.sample_count = count;
  config.message = "rosef-batch";

  if (!batchRun(config)) {
    Serial.println(F("ERR BATCH_FAILED"));
    return;
  }

  Serial.println(F("OK BATCH=PASS"));
}

void handleIna219() {
  const Ina219Reading reading = ina219Read();

  if (!reading.valid) {
    Serial.println(F("ERR INA219_UNAVAILABLE"));
    return;
  }

  Serial.print(F("OK INA219"));
  Serial.print(F(" V="));
  Serial.print(reading.bus_voltage_v, 6);
  Serial.print(F(" SHUNT_MV="));
  Serial.print(reading.shunt_voltage_mv, 6);
  Serial.print(F(" I_MA="));
  Serial.print(reading.current_ma, 6);
  Serial.print(F(" P_MW="));
  Serial.println(reading.power_mw, 6);
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

void handleLine(
    char *command) {

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

  if (strcmp(
          command,
          "SELFTEST") == 0) {

    if (selfTestRun()) {
      Serial.println(
          F("OK SELFTEST=PASS"));
    } else {
      Serial.println(
          F("ERR SELFTEST=FAIL"));
    }

    return;
  }

  if (strcmp(
          command,
          "HELP") == 0) {

    printIntro();
    return;
  }

  // ----------------------------------------------------------
  // TEMPORARY CONNECTIVITY TEST
  // ----------------------------------------------------------
  if (strcmp(
          command,
          "TESTPR") == 0) {

    handleTestPR();
    return;
  }

  if (strcmp(
          command,
          "INA219") == 0) {

    ina219FindBlocking();
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

  if (strncmp(
          command,
          "BATCH ",
          6) == 0) {

    handleBatch(
        command + 6);
    return;
  }

  if (*command != '\0') {
    Serial.println(
        F("ERR UNKNOWN_COMMAND"));
  }
}

}

void protocolBegin() {
  Serial.begin(SERIAL_BAUD);

  experimentBegin();
  buttonBegin();

  delay(250);

  printIntro();

  Serial.println(
      F("READY ROSEF_ECDSA_EXP"));

  printInfo();
}

void protocolLoop() {
  buttonIsPressed();

  while (Serial.available() > 0) {
    const int value =
        Serial.read();

    if (value < 0) {
      return;
    }

    const char c =
        static_cast<char>(value);

    if (c == '\r' ||
        c == '\n') {

      if (line_length == 0) {
        continue;
      }

      line[line_length] =
          '\0';

      handleLine(line);

      line_length = 0;

      continue;
    }

    if (line_length + 1 >=
        sizeof(line)) {

      line_length = 0;

      Serial.println(
          F("ERR LINE_TOO_LONG"));

      continue;
    }

    line[line_length++] = c;
  }
}

}