#include "experiment.h"

#include <cstring>

#include "config.h"
#include "ecdsa_exp.h"
#include "mbedtls/sha256.h"

namespace rosef {

namespace {

/*
 * TPK
*/
uint8_t g_private_key[32] = {
  0x1c, 0x3a, 0x7b, 0x2e, 0x44, 0x11, 0x90, 0xaa,
  0x12, 0x33, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
  0x10, 0x22, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde,
  0xf0, 0x13, 0x57, 0x9b, 0xdf, 0x24, 0x68, 0xac
};

uint8_t g_default_nonce[32] = {
  0x2a, 0x19, 0x83, 0x71, 0x55, 0x44, 0x32, 0x10,
  0xfe, 0xdc, 0xba, 0x98, 0x76, 0x54, 0x32, 0x10,
  0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88,
  0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff, 0x01
};

}

void experimentBegin() {
  pinMode(TRIGGER_GPIO, OUTPUT);
  digitalWrite(TRIGGER_GPIO, LOW);
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

  digitalWrite(TRIGGER_GPIO, HIGH);

  const int rc = ecdsa_sign_fixed_k(
      g_private_key,
      nonce,
      digest,
      result.r,
      result.s);

  digitalWrite(TRIGGER_GPIO, LOW);

  result.elapsed_us = micros() - start_us;

  return rc == 0;
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

bool experimentSelfTest() {
  static const char EXPECTED_R[] =
      "d4087e276af6dfccacd644bcb5b19b4d43cfc50e6786d97d38a949549ee2bf36";

  static const char EXPECTED_S[] =
      "3705303a0eb0b67bda71fb8f3f5a2b2f520b483fd9b35f09ccc26ae06f529b09";

  uint8_t digest[32];
  SignResult result;

  if (!experimentSha256("hello", digest)) {
    return false;
  }

  if (!experimentSignDigest(
          digest,
          g_default_nonce,
          result)) {
    return false;
  }

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

  return
      strcmp(r_hex, EXPECTED_R) == 0 &&
      strcmp(s_hex, EXPECTED_S) == 0;
}

uint8_t *experimentPrivateKey() {
  return g_private_key;
}

uint8_t *experimentDefaultNonce() {
  return g_default_nonce;
}

}
