#include "experiment.h"

#include <cstring>

#include "../core/config.h"
#include "../ina219/ina219.h"
#include "../crypto/ecdsa_exp.h"
#include "../test_vectors/test_vectors.h"
#include "mbedtls/sha256.h"

namespace rosef {

namespace {

 


uint8_t g_private_key[32];
uint8_t g_default_nonce[32];
bool g_trigger_enabled = true;

}   

void experimentBegin() {
  memcpy(g_private_key, TEST_PRIVATE_KEY, sizeof(g_private_key));
  memcpy(g_default_nonce, TEST_DEFAULT_NONCE, sizeof(g_default_nonce));
  pinMode(TRIGGER_GPIO, OUTPUT);
  digitalWrite(TRIGGER_GPIO, TRIGGER_IDLE_LEVEL);
  g_trigger_enabled = true;
  ina219Begin();
}

void experimentSetTriggerEnabled(bool enabled) {
  g_trigger_enabled = enabled;
   
   
  pinMode(TRIGGER_GPIO, OUTPUT);
  digitalWrite(TRIGGER_GPIO, TRIGGER_IDLE_LEVEL);
}

bool experimentSha256(
    const char *message,
    uint8_t digest[32]) {

  if (message == nullptr || digest == nullptr) {
    return false;
  }

  const int rc = mbedtls_sha256(
      reinterpret_cast<const unsigned char *>(message),
      strlen(message),
      digest,
      0);

  return rc == 0;
}

bool experimentSignDigest(
    const uint8_t digest[32],
    const uint8_t nonce[32],
    SignResult &result) {

  if (digest == nullptr || nonce == nullptr) {
    return false;
  }

  memset(&result, 0, sizeof(result));

  const uint32_t start_us = micros();

   
   
   
   
  const bool trigger_enabled = g_trigger_enabled;
  result.trigger_enabled = trigger_enabled;
  if (trigger_enabled) {
    pinMode(TRIGGER_GPIO, OUTPUT);
    digitalWrite(TRIGGER_GPIO, TRIGGER_IDLE_LEVEL);
    digitalWrite(TRIGGER_GPIO, TRIGGER_ACTIVE_LEVEL);
    result.trigger_low_readback_ok =
        digitalRead(TRIGGER_GPIO) == TRIGGER_ACTIVE_LEVEL;
  }
  result.trigger_start_us = micros();

  const int rc = ecdsa_sign_fixed_k(
      g_private_key,
      nonce,
      digest,
      result.r,
      result.s);

   
  if (trigger_enabled) {
    digitalWrite(TRIGGER_GPIO, TRIGGER_IDLE_LEVEL);
    result.trigger_high_readback_ok =
        digitalRead(TRIGGER_GPIO) == TRIGGER_IDLE_LEVEL;
  }
  result.trigger_end_us = micros();
  result.elapsed_us = micros() - start_us;

  return rc == 0;
}

bool experimentVerifyDigest(
    const uint8_t digest[32],
    const SignResult &result) {

  if (digest == nullptr) {
    return false;
  }

  return ecdsa_verify_with_private_key(
      g_private_key,
      digest,
      result.r,
      result.s);
}

bool experimentSignMessage(
    const char *message,
    const uint8_t nonce[32],
    SignResult &result) {

  uint8_t digest[32];

  if (!experimentSha256(message, digest)) {
    memset(&result, 0, sizeof(result));
    return false;
  }

  return experimentSignDigest(
      digest,
      nonce,
      result);
}

uint8_t *experimentPrivateKey() {
  return g_private_key;
}

uint8_t *experimentDefaultNonce() {
  return g_default_nonce;
}

}
