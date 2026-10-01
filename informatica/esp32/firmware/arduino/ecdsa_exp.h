#pragma once

#include <Arduino.h>

namespace rosef {

/**
 * private_key: 32byte scalar d
 * nonce:       32byte scalar k
 * digest:      32byte message digest z
 * r_out:       32byte big-endian ECDSA r
 * s_out:       32byte big-endian ECDSA s
 * 0 - succes, anything else gg
 */
int ecdsa_sign_fixed_k(
    const uint8_t private_key[32],
    const uint8_t nonce[32],
    const uint8_t digest[32],
    uint8_t r_out[32],
    uint8_t s_out[32]);

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
