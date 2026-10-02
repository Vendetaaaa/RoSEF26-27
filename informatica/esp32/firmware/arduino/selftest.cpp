#include "selftest.h"

#include <Arduino.h>
#include <cstring>

#include "ecdsa_exp.h"
#include "experiment.h"

namespace rosef {

namespace {

bool testEcdsaVector() {
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
          experimentDefaultNonce(),
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

  if (strcmp(r_hex, EXPECTED_R) != 0) {
    return false;
  }

  if (strcmp(s_hex, EXPECTED_S) != 0) {
    return false;
  }

  return true;
}

bool testDeterminism() {
  uint8_t digest[32];
  SignResult first;
  SignResult second;

  if (!experimentSha256("hello", digest)) {
    return false;
  }

  if (!experimentSignDigest(
          digest,
          experimentDefaultNonce(),
          first)) {
    return false;
  }

  if (!experimentSignDigest(
          digest,
          experimentDefaultNonce(),
          second)) {
    return false;
  }

  return
      memcmp(first.r, second.r, sizeof(first.r)) == 0 &&
      memcmp(first.s, second.s, sizeof(first.s)) == 0;
}

bool testDifferentMessage() {
  uint8_t digest_hello[32];
  uint8_t digest_test[32];

  SignResult hello_result;
  SignResult test_result;

  if (!experimentSha256("hello", digest_hello)) {
    return false;
  }

  if (!experimentSha256("test", digest_test)) {
    return false;
  }

  if (!experimentSignDigest(
          digest_hello,
          experimentDefaultNonce(),
          hello_result)) {
    return false;
  }

  if (!experimentSignDigest(
          digest_test,
          experimentDefaultNonce(),
          test_result)) {
    return false;
  }

  if (memcmp(
          hello_result.r,
          test_result.r,
          sizeof(hello_result.r)) != 0) {
    return false;
  }

  if (memcmp(
          hello_result.s,
          test_result.s,
          sizeof(hello_result.s)) == 0) {
    return false;
  }

  return true;
}

bool testSha256() {
  static const char EXPECTED_DIGEST[] =
      "2cf24dba5fb0a30e26e83b2ac5b9e29e"
      "1b161e5c1fa7425e73043362938b9824";

  uint8_t digest[32];
  char digest_hex[65];

  if (!experimentSha256("hello", digest)) {
    return false;
  }

  bytesToHex(
      digest,
      sizeof(digest),
      digest_hex,
      sizeof(digest_hex));

  return strcmp(
      digest_hex,
      EXPECTED_DIGEST) == 0;
}

}

bool selfTestRun() {
  if (!testSha256()) {
    return false;
  }

  if (!testEcdsaVector()) {
    return false;
  }

  if (!testDeterminism()) {
    return false;
  }

  if (!testDifferentMessage()) {
    return false;
  }

  return true;
}

}
