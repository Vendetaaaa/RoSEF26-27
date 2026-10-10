#include "protocol.h"

#include <Arduino.h>
#include <cstdlib>
#include <cmath>
#include <cstring>
#include <cfloat>
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
#include "../test_vectors/test_vectors.h"

namespace rosef {

namespace {

char line[MAX_SERIAL_LINE];
size_t line_length = 0;

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

  Serial.println(F("TESTPR END"));
}

constexpr uint32_t MAX_PULSE_US = 1000000;

void printIntro() {
  Serial.println(F("COMMANDS"));
  Serial.println(F("PING - testa conexiunea"));
  Serial.println(F("INFO - arata configuratia"));
  Serial.println(F("STATUS - arata starea"));
  Serial.println(F("SELFTEST - ruleaza autotestul"));
  Serial.println(F("FULLIMP - V2 capture sincronizat ECDSA + INA219 (test vectors only)"));
  Serial.println(F("SETK - seteaza nonce-ul"));
  Serial.println(F("SETD - seteaza cheia"));
  Serial.println(F("SIGN - semneaza mesajul"));
  Serial.println(F("SIGNHEX - semneaza digestul"));
  Serial.println(F("SIGNK - semneaza cu nonce"));
  Serial.println(F("SIGNHEXK - semneaza digest+nonce"));
  Serial.println(F("BENCH - ruleaza benchmark-ul"));
  Serial.println(F("BATCH - ruleaza testul batch"));
  Serial.println(F("BATCH INA219 <quant> - citeste INA219 de quant ori"));
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
  if (!ina219Available() && !ina219FindOnce()) {
    Serial.println(F("ERR INA219_UNAVAILABLE"));
    return;
  }

  const Ina219Reading reading = ina219Read();
  if (!reading.valid) {
    Serial.println(F("ERR INA219_UNAVAILABLE"));
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
  Serial.println(reading.power_mw, 6);
}

void handleBatchIna219(const char *argument) {
  uint32_t count = 0;
  if (!parseUnsigned(argument, count, MAX_BATCH_COUNT)) {
    Serial.println(F("ERR BAD_BATCH_COUNT"));
    return;
  }

  if (!ina219Available() && !ina219FindOnce()) {
    Serial.println(F("ERR INA219_UNAVAILABLE"));
    return;
  }

  Serial.println(F("BEGIN BATCH INA219"));
  Serial.print(F("META COUNT="));
  Serial.println(count);

  for (uint32_t i = 0; i < count; ++i) {
    const Ina219Reading reading = ina219Read();
    if (!reading.valid) {
      Serial.print(F("ERR INA219_READ ID="));
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
    Serial.println(reading.power_mw, 6);
    delay(1);
  }

  Serial.println(F("END BATCH INA219"));
  Serial.println(F("OK BATCH INA219=PASS"));
}

constexpr uint32_t FULLIMP_BATCH_COUNT = 100;
constexpr float FULLIMP_NOMINAL_LOAD_OHMS = 330.0f;
constexpr uint32_t FULLIMP_MAX_SAMPLES = 4096;
constexpr uint32_t FULLIMP_IDLE_TARGET = 20;
constexpr uint32_t FULLIMP_SAMPLE_PERIOD_MS = 8;
constexpr uint8_t FULLIMP_CAPTURE_NONE = 0;
constexpr uint8_t FULLIMP_CAPTURE_IDLE = 1;
constexpr uint8_t FULLIMP_CAPTURE_BATCH = 2;
constexpr uint8_t FULLIMP_PHASE_IDLE = 1;
constexpr uint8_t FULLIMP_PHASE_ECDSA = 2;

struct FullImpSample {
  uint32_t timestamp_us;
  float voltage_v;
  float current_ma;
  float power_mw;
  uint16_t operation;
  uint8_t phase;
};

struct FullImpOpStats {
  uint32_t sign_us;
  uint32_t trigger_start_rel_us;
  uint32_t trigger_end_rel_us;
  uint16_t sample_count;
  bool sign_ok;
  bool verify_ok;
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

void fullImpSamplerTask(void * /*parameter*/) {
  g_fullimp_sampler_running = true;
  TickType_t sample_delay = pdMS_TO_TICKS(FULLIMP_SAMPLE_PERIOD_MS);
  if (sample_delay == 0) sample_delay = 1;

  while (!g_fullimp_sampler_stop) {
    const uint8_t capture_mode = g_fullimp_capture_mode;
    if (capture_mode != FULLIMP_CAPTURE_NONE) {
      const uint16_t operation = capture_mode == FULLIMP_CAPTURE_BATCH
          ? g_fullimp_current_operation : 0;
      const bool trigger_active =
          digitalRead(TRIGGER_GPIO) == TRIGGER_ACTIVE_LEVEL;
      const uint32_t timestamp_us = micros() - g_fullimp_capture_start_us;
      const Ina219Reading reading = ina219Read();

      uint8_t phase = 0;
      if (capture_mode == FULLIMP_CAPTURE_IDLE) {
        phase = FULLIMP_PHASE_IDLE;
      } else if (capture_mode == FULLIMP_CAPTURE_BATCH && trigger_active) {
        phase = FULLIMP_PHASE_ECDSA;
      }

      if (!reading.valid) {
        ++g_fullimp_sample_read_errors;
      } else if (phase != 0) {
        const uint32_t index = g_fullimp_sample_count;
        if (index < FULLIMP_MAX_SAMPLES) {
          FullImpSample &sample = g_fullimp_samples[index];
          sample.timestamp_us = timestamp_us;
          sample.voltage_v = reading.bus_voltage_v;
          sample.current_ma = reading.current_ma;
          sample.power_mw = reading.power_mw;
          sample.operation = phase == FULLIMP_PHASE_ECDSA ? operation : 0;
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

    vTaskDelay(sample_delay);
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

void incrementTestNonce(uint8_t nonce[32]) {
  for (int k = 31; k >= 0; --k) {
    ++nonce[k];
    if (nonce[k] != 0) break;
  }
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

void handleFullImp() {
  bool all_ok = true;
  Serial.println(F("BEGIN FULLIMP V2"));
  Serial.println(F("MODE=TEST_VECTORS_ONLY NONCE_POLICY=INCREMENTING_TEST_NONCE"));

  const bool test_vectors_ok =
      memcmp(experimentPrivateKey(), TEST_PRIVATE_KEY, 32) == 0 &&
      memcmp(experimentDefaultNonce(), TEST_DEFAULT_NONCE, 32) == 0;
  Serial.println(test_vectors_ok
      ? F("CHECK TEST_VECTORS=PASS")
      : F("CHECK TEST_VECTORS=FAIL (FULLIMP is test-only; restore test vectors first)"));
  if (!test_vectors_ok) {
    Serial.println(F("END FULLIMP"));
    Serial.println(F("ERR FULLIMP=TEST_VECTORS_REQUIRED"));
    return;
  }

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
  Serial.print(F("CHECK TRIGGER_GPIO="));
  Serial.print(TRIGGER_GPIO);
  Serial.println(F("=CONFIGURED (physical Logic Analyzer capture must be checked in PulseView)"));

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
    Serial.println(F("BATCH SKIPPED: required pre-check failed"));
    Serial.println(F("END FULLIMP"));
    Serial.println(F("ERR FULLIMP=FAIL"));
    return;
  }

  uint8_t digest[32];
  if (!experimentSha256("FULLIMP-BATCH-100-V2", digest)) {
    Serial.println(F("ERR FULLIMP_DIGEST"));
    Serial.println(F("END FULLIMP"));
    Serial.println(F("ERR FULLIMP=FAIL"));
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
    Serial.println(F("CHECK INA219_SAMPLER=FAIL TASK_CREATE"));
    Serial.println(F("END FULLIMP"));
    Serial.println(F("ERR FULLIMP=FAIL"));
    return;
  }

  const uint32_t sampler_start_wait = millis();
  while (!g_fullimp_sampler_running &&
         static_cast<uint32_t>(millis() - sampler_start_wait) < 1000) {
    delay(1);
  }
  if (!g_fullimp_sampler_running) {
    g_fullimp_sampler_stop = true;
    Serial.println(F("CHECK INA219_SAMPLER=FAIL TASK_START_TIMEOUT"));
    Serial.println(F("END FULLIMP"));
    Serial.println(F("ERR FULLIMP=FAIL"));
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
    Serial.println(F("END FULLIMP"));
    Serial.println(F("ERR FULLIMP=FAIL"));
    return;
  }

  uint8_t nonce[32];
  memcpy(nonce, experimentDefaultNonce(), sizeof(nonce));
  incrementTestNonce(nonce);

  Serial.println(F("CHECK INA219_SAMPLER=PASS CORE=0 BUFFER=4096"));
  Serial.println(F("BEGIN FULLIMP BATCH=100 MODE=TIMESTAMPED_ECDSA_TRIGGER+INA219"));
  Serial.println(F("SAMPLE_PERIOD_MS=8 (actual interval depends on I2C transaction time)"));
  Serial.println(F("THEORY MODEL: I_MA = BUS_VOLTAGE_V / 330_OHM * 1000; nominal-load estimate only"));
  g_fullimp_capture_mode = FULLIMP_CAPTURE_BATCH;

  uint32_t completed = 0;
  uint32_t verify_pass_count = 0;
  for (uint32_t i = 0; i < FULLIMP_BATCH_COUNT; ++i) {
    g_fullimp_current_operation = static_cast<uint16_t>(i + 1);
    SignResult sign_result{};
    const bool sign_ok = experimentSignDigest(digest, nonce, sign_result);
    FullImpOpStats &op_stats = g_fullimp_op_stats[i];
    op_stats.sign_ok = sign_ok;
    op_stats.sign_us = sign_result.elapsed_us;
    op_stats.trigger_start_rel_us = sign_result.trigger_start_us - g_fullimp_capture_start_us;
    op_stats.trigger_end_rel_us = sign_result.trigger_end_us - g_fullimp_capture_start_us;

    if (!sign_ok) {
      Serial.print(F("BATCH ")); Serial.print(i + 1);
      Serial.println(F(": ECDSA_SIGN=FAIL"));
      all_ok = false;
      break;
    }

    op_stats.verify_ok = experimentVerifyDigest(digest, sign_result);
    if (op_stats.verify_ok) {
      ++verify_pass_count;
    } else {
      all_ok = false;
    }
    ++completed;
    incrementTestNonce(nonce);
    delay(1);
  }

  const bool sampler_stopped = stopFullImpSampler();
  if (!sampler_stopped) {
    Serial.println(F("CHECK INA219_SAMPLER=FAIL STOP_TIMEOUT"));
    all_ok = false;
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
    if (op.sample_count == 0 || !op.verify_ok) all_ok = false;
  }
  if (g_fullimp_sample_read_errors != 0 ||
      g_fullimp_sample_overflows != 0 ||
      completed != FULLIMP_BATCH_COUNT ||
      active_sample_count == 0 ||
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
  Serial.println(F("BEGIN FULLIMP OPERATION SUMMARY"));
  for (uint32_t i = 0; i < completed; ++i) {
    const FullImpOpStats &op = g_fullimp_op_stats[i];
    const double op_avg_current = op.sample_count
        ? op.current_sum / op.sample_count : 0.0;
    const double op_avg_voltage = op.sample_count
        ? op.voltage_sum / op.sample_count : 0.0;
    const double op_avg_power = op.sample_count
        ? op.power_sum / op.sample_count : 0.0;

    Serial.print(F("BATCH ")); Serial.print(i + 1);
    Serial.print(F(": ECDSA_SIGN=")); Serial.print(op.sign_ok ? F("PASS") : F("FAIL"));
    Serial.print(F(" ECDSA_VERIFY=")); Serial.print(op.verify_ok ? F("PASS") : F("FAIL"));
    Serial.print(F(" SIGN_US=")); Serial.print(op.sign_us);
    Serial.print(F(" TRIGGER_START_T_US=")); Serial.print(op.trigger_start_rel_us);
    Serial.print(F(" TRIGGER_END_T_US=")); Serial.print(op.trigger_end_rel_us);
    Serial.print(F(" CAPTURE_SAMPLES=")); Serial.print(op.sample_count);
    Serial.print(F(" INA219: AVG_V=")); Serial.print(op_avg_voltage, 6);
    Serial.print(F(" AVG_I_MA=")); Serial.print(op_avg_current, 6);
    Serial.print(F(" MAX_SAMPLED_I_MA=")); Serial.print(op.peak_current_ma, 6);
    Serial.print(F(" AVG_P_MW=")); Serial.println(op_avg_power, 6);
  }
  Serial.println(F("END FULLIMP OPERATION SUMMARY"));

  Serial.println(F("BEGIN FULLIMP IDLE SAMPLES"));
  for (uint32_t i = 0; i < g_fullimp_sample_count; ++i) {
    const FullImpSample &sample = g_fullimp_samples[i];
    if (sample.phase != FULLIMP_PHASE_IDLE) continue;
    Serial.print(F("IDLE_SAMPLE T_US=")); Serial.print(sample.timestamp_us);
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
    Serial.print(F(" TRIGGER=ACTIVE_LOW T_US=")); Serial.print(sample.timestamp_us);
    Serial.print(F(" V=")); Serial.print(sample.voltage_v, 6);
    Serial.print(F(" I_MA=")); Serial.print(sample.current_ma, 6);
    Serial.print(F(" P_MW=")); Serial.println(sample.power_mw, 6);
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
  Serial.println(F("NOTE: MAX_SAMPLED is the largest value observed by INA219, not a high-speed instantaneous peak."));
  Serial.println(F("NOTE: 330-ohm theory is an estimate; physical Logic Analyzer capture must be validated in PulseView."));

  Serial.println(F("END FULLIMP"));
  Serial.println(all_ok && completed == FULLIMP_BATCH_COUNT &&
          idle_sample_count >= FULLIMP_IDLE_TARGET
      ? F("OK FULLIMP=PASS") : F("ERR FULLIMP=FAIL"));
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

  if (strcmp(command, "FULLIMP") == 0) {
    handleFullImp();
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

  if (strcmp(
          command,
          "TESTPR") == 0) {

    handleTestPR();
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