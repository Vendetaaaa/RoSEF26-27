#pragma once

#include <Arduino.h>

namespace rosef {

struct SignResult {
  uint8_t r[32];
  uint8_t s[32];
  uint32_t elapsed_us;
};

void experimentBegin();

bool experimentSha256(
    const char *message,
    uint8_t digest[32]);

bool experimentSignDigest(
    const uint8_t digest[32],
    const uint8_t nonce[32],
    SignResult &result);

bool experimentSignMessage(
    const char *message,
    const uint8_t nonce[32],
    SignResult &result);


uint8_t *experimentPrivateKey();

uint8_t *experimentDefaultNonce();

}
