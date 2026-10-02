#include "protocol.h"

#include <Arduino.h>
#include <cstdlib>
#include <cstring>

#include "config.h"
#include "ecdsa_exp.h"
#include "experiment.h"
#include "benchmark.h"

namespace rosef {

namespace {

char line[MAX_SERIAL_LINE];
size_t line_length = 0;

constexpr uint32_t MAX_PULSE_US = 1000000;
constexpr uint32_t MAX_BENCH_COUNT = 1000;

void printHelp() {
  Serial.println(F("OK COMMANDS"));
  Serial.println(F("PING"));
  Serial.println(F("INFO"));
  Serial.println(F("STATUS"));
  Serial.println(F("SELFTEST"));
  Serial.println(F("SETK <64hex>"));
  Serial.println(F("SETD <64hex>"));
  Serial.println(F("SIGN <ascii-message>"));
  Serial.println(F("SIGNHEX <64hex-digest>"));
  Serial.println(F("SIGNK <64hex-k> <ascii-message>"));
  Serial.println(F("SIGNHEXK <64hex-k> <64hex-digest>"));
  Serial.println(F("BENCH <count>"));
  Serial.println(F("PULSE"));
  Serial.println(F("PULSE <microseconds>"));
  Serial.println(F("HELP"));
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
  Serial.println(
      digitalRead(TRIGGER_GPIO) ? F("HIGH") : F("LOW"));
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

  value_out = static_cast<uint32_t>(parsed);
  return true;
}

void printSignResult(const SignResult &result) {
  char r_hex[65];
  char s_hex[65];

  bytesToHex(
      result.r,
      sizeof(result.r),
      r_hex,
      sizeof(r_hex));

  bytesToHex(
      result.s,
      sizeof(result.s),
      s_hex,
      sizeof(s_hex));

  Serial.print(F("OK R="));
  Serial.print(r_hex);

  Serial.print(F(" S="));
  Serial.print(s_hex);

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

bool parseNonce(
    const char *hex,
    uint8_t nonce[32]) {

  return hexToBytes(
      hex,
      nonce,
      32);
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

void handleSetK(const char *hex) {
  uint8_t *nonce = experimentDefaultNonce();

  if (!hexToBytes(
          hex,
          nonce,
          32)) {
    Serial.println(F("ERR BAD_K"));
    return;
  }

  Serial.println(F("OK K_SET"));
}

void handleSetD(const char *hex) {
  uint8_t *private_key = experimentPrivateKey();

  if (!hexToBytes(
          hex,
          private_key,
          32)) {
    Serial.println(F("ERR BAD_D"));
    return;
  }

  Serial.println(F("OK D_SET"));
}

void handleSignHex(const char *hex) {
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

void handleSign(const char *message) {
  handleSignMessage(
      message,
      experimentDefaultNonce());
}

void handleSignK(char *payload) {
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

  memcpy(k_hex, payload, 64);
  k_hex[64] = '\0';

  uint8_t nonce[32];

  if (!parseNonce(k_hex, nonce)) {
    Serial.println(F("ERR BAD_K"));
    return;
  }

  const char *message = payload + 65;

  handleSignMessage(
      message,
      nonce);
}

void handleSignHexK(char *payload) {
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

  memcpy(k_hex, payload, 64);
  k_hex[64] = '\0';

  memcpy(
      digest_hex,
      payload + 65,
      64);

  digest_hex[64] = '\0';

  uint8_t nonce[32];
  uint8_t digest[32];

  if (!parseNonce(k_hex, nonce)) {
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

void handlePulse(const char *argument) {
  uint32_t width_us = DEFAULT_TRIGGER_PULSE_US;

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

  const uint32_t start_us = micros();

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

void handleBenchmark(const char *argument) {
  uint32_t count = 0;

  if (!parseUnsigned(
          argument,
          count,
          MAX_BENCH_COUNT)) {
    Serial.println(F("ERR BAD_BENCH_COUNT"));
    return;
  }

  BenchmarkResult result;

  if (!benchmarkRun(count, result)) {
    Serial.println(F("ERR BENCH_FAILED"));
    return;
  }

  Serial.print(F("OK BENCH"));

  Serial.print(F(" N="));
  Serial.print(result.count);

  Serial.print(F(" TOTAL_US="));
  Serial.print(
      static_cast<unsigned long>(result.total_us));

  Serial.print(F(" AVG_US="));
  Serial.print(result.average_us);

  Serial.print(F(" MIN_US="));
  Serial.print(result.min_us);

  Serial.print(F(" MAX_US="));
  Serial.println(result.max_us);
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

  if (strcmp(command, "SELFTEST") == 0) {
    if (experimentSelfTest()) {
      Serial.println(F("OK SELFTEST ECDSA=PASS"));
    } else {
      Serial.println(F("ERR SELFTEST ECDSA=FAIL"));
    }
    return;
  }

  if (strcmp(command, "HELP") == 0) {
    printHelp();
    return;
  }

  if (strcmp(command, "PULSE") == 0) {
    handlePulse(nullptr);
    return;
  }

  if (strncmp(command, "PULSE ", 6) == 0) {
    handlePulse(command + 6);
    return;
  }

  if (strncmp(command, "SETK ", 5) == 0) {
    handleSetK(command + 5);
    return;
  }

  if (strncmp(command, "SETD ", 5) == 0) {
    handleSetD(command + 5);
    return;
  }

  if (strncmp(command, "SIGNHEXK ", 9) == 0) {
    handleSignHexK(command + 9);
    return;
  }

  if (strncmp(command, "SIGNK ", 6) == 0) {
    handleSignK(command + 6);
    return;
  }

  if (strncmp(command, "SIGNHEX ", 8) == 0) {
    handleSignHex(command + 8);
    return;
  }

  if (strncmp(command, "SIGN ", 5) == 0) {
    handleSign(command + 5);
    return;
  }

  if (strncmp(command, "BENCH ", 6) == 0) {
    handleBenchmark(command + 6);
    return;
  }

  if (*command != '\0') {
    Serial.println(F("ERR UNKNOWN_COMMAND"));
  }
}

}

void protocolBegin() {
  Serial.begin(SERIAL_BAUD);

  experimentBegin();

  delay(250);

  Serial.println(
      F("READY ROSEF_ECDSA_EXP"));

  printInfo();
  printHelp();
}

void protocolLoop() {
  while (Serial.available() > 0) {
    const int value = Serial.read();

    if (value < 0) {
      return;
    }

    const char c =
        static_cast<char>(value);

    if (c == '\r' || c == '\n') {
      if (line_length == 0) {
        continue;
      }

      line[line_length] = '\0';

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
