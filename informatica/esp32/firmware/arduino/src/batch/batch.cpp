#include "batch.h"

#include <Arduino.h>
#include <cstring>

#include "../core/config.h"
#include "../crypto/ecdsa_exp.h"
#include "../experiment/experiment.h"

namespace rosef {

namespace {

bool scalarIsInSecp256k1Range(const uint8_t scalar[32]) {
  static const uint8_t order[32] = {
      0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFE,
      0xBA, 0xAE, 0xDC, 0xE6, 0xAF, 0x48, 0xA0, 0x3B,
      0xBF, 0xD2, 0x5E, 0x8C, 0xD0, 0x36, 0x41, 0x41};
  uint8_t nonzero = 0;
  for (size_t i = 0; i < 32; ++i) nonzero |= scalar[i];
  return nonzero != 0 && memcmp(scalar, order, 32) < 0;
}

void incrementNonce(uint8_t nonce[32]) {
  for (int i = 31; i >= 0; --i) {
    ++nonce[i];
    if (nonce[i] != 0) break;
  }
  if (!scalarIsInSecp256k1Range(nonce)) {
    memset(nonce, 0, 32);
    nonce[31] = 1;
  }
}

void printHex(
    const uint8_t *data,
    size_t length) {

  char buffer[65];

  if (length > 32) {
    return;
  }

  bytesToHex(
      data,
      length,
      buffer,
      sizeof(buffer));

  Serial.print(buffer);
}

}

bool batchRun(
    const BatchConfig &config) {

  if (config.sample_count == 0 ||
      config.sample_count > MAX_BATCH_COUNT) {
    return false;
  }

  if (config.message == nullptr ||
      config.message[0] == '\0') {
    return false;
  }

  uint8_t digest[32];

  if (!experimentSha256(
          config.message,
          digest)) {
    return false;
  }

  uint8_t nonce[32];
  memcpy(nonce, experimentDefaultNonce(), sizeof(nonce));
  if (!scalarIsInSecp256k1Range(nonce)) {
    Serial.println(F("ERR BATCH_BAD_START_NONCE (use SETK with 1 <= K < secp256k1 order)"));
    return false;
  }

  experimentSetTriggerEnabled(true);

  Serial.println(F("BEGIN BATCH"));
  Serial.print(F("META COUNT=")); Serial.print(config.sample_count);
  Serial.print(F(" MESSAGE=")); Serial.println(config.message);
  Serial.print(F("META DIGEST=")); printHex(digest, sizeof(digest)); Serial.println();

  uint32_t verify_pass_count = 0;
  uint32_t trigger_readback_pass_count = 0;
  for (uint32_t i = 0; i < config.sample_count; ++i) {
    SignResult result{};
    if (!experimentSignDigest(digest, nonce, result)) {
      Serial.print(F("ERR BATCH_SIGN ID=")); Serial.println(i);
      Serial.println(F("END BATCH"));
      return false;
    }

    const bool verify_ok = experimentVerifyDigest(digest, result);
    const bool trigger_ok = result.trigger_enabled &&
        result.trigger_low_readback_ok && result.trigger_high_readback_ok;
    if (verify_ok) ++verify_pass_count;
    if (trigger_ok) ++trigger_readback_pass_count;

    Serial.print(F("SAMPLE ID=")); Serial.print(i);
    Serial.print(F(" OP=")); Serial.print(i + 1);
    Serial.print(F(" K=")); printHex(nonce, sizeof(nonce));
    Serial.print(F(" R=")); printHex(result.r, sizeof(result.r));
    Serial.print(F(" S=")); printHex(result.s, sizeof(result.s));
    Serial.print(F(" DT_US=")); Serial.print(result.elapsed_us);
    Serial.print(F(" VERIFY=")); Serial.print(verify_ok ? F("PASS") : F("FAIL"));
    Serial.print(F(" TRIGGER_LOW_READBACK=")); Serial.print(result.trigger_low_readback_ok ? F("PASS") : F("FAIL"));
    Serial.print(F(" TRIGGER_HIGH_READBACK=")); Serial.println(result.trigger_high_readback_ok ? F("PASS") : F("FAIL"));

    if (!verify_ok) {
      Serial.print(F("ERR BATCH_VERIFY ID=")); Serial.println(i);
      Serial.println(F("END BATCH"));
      return false;
    }
    if (!trigger_ok) {
      Serial.print(F("ERR BATCH_TRIGGER_READBACK ID=")); Serial.println(i);
      Serial.println(F("END BATCH"));
      return false;
    }

    incrementNonce(nonce);
    delay(1);
  }

  Serial.println(F("END BATCH"));
  Serial.print(F("SUMMARY COUNT=")); Serial.print(config.sample_count);
  Serial.print(F(" VERIFY_PASS_COUNT=")); Serial.print(verify_pass_count);
  Serial.print(F(" TRIGGER_READBACK_PASS_COUNT=")); Serial.println(trigger_readback_pass_count);
  return verify_pass_count == config.sample_count &&
      trigger_readback_pass_count == config.sample_count;
}


}
