#pragma once

#include <Arduino.h>

namespace rosef {

int ecdsa_sign_fixed_k(
    const uint8_t private_key[32],
    const uint8_t nonce[32],
    const uint8_t digest[32],
    uint8_t r_out[32],
    uint8_t s_out[32]);

bool ecdsa_verify_with_private_key(
    const uint8_t private_key[32],
    const uint8_t digest[32],
    const uint8_t r_in[32],
    const uint8_t s_in[32]);

void bytesToHex(
    const uint8_t *data,
    size_t length,
    char *output,
    size_t output_size);

bool hexToBytes(
    const char *hex,
    uint8_t *output,
    size_t output_length);

}
