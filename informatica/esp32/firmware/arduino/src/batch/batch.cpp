#include "batch.h"

#include <Arduino.h>
#include <cstring>

#include "../crypto/ecdsa_exp.h"
#include "../experiment/experiment.h"

namespace rosef {

namespace {

constexpr uint32_t MAX_BATCH_COUNT = 1000;

void incrementNonce(uint8_t nonce[32]) {
  for (int i = 31; i >= 0; --i) {
    ++nonce[i];

    if (nonce[i] != 0) {
      return;
    }
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

  memcpy(
      nonce,
      experimentDefaultNonce(),
      sizeof(nonce));

  Serial.println(F("BEGIN BATCH"));

  Serial.print(F("META COUNT="));
  Serial.print(config.sample_count);

  Serial.print(F(" MESSAGE="));
  Serial.println(config.message);

  Serial.print(F("META DIGEST="));
  printHex(digest, sizeof(digest));
  Serial.println();

  for (uint32_t i = 0;
       i < config.sample_count;
       ++i) {

    SignResult result;

    if (!experimentSignDigest(
            digest,
            nonce,
            result)) {

      Serial.print(F("ERR BATCH_SIGN ID="));
      Serial.println(i);

      return false;
    }

    Serial.print(F("SAMPLE"));

    Serial.print(F(" ID="));
    Serial.print(i);

    Serial.print(F(" K="));
    printHex(
        nonce,
        sizeof(nonce));

    Serial.print(F(" R="));
    printHex(
        result.r,
        sizeof(result.r));

    Serial.print(F(" S="));
    printHex(
        result.s,
        sizeof(result.s));

    Serial.print(F(" DT_US="));
    Serial.println(result.elapsed_us);

    incrementNonce(nonce);

    delay(1);
  }

  Serial.println(F("END BATCH"));

  return true;
}

}  // namespace rosef
