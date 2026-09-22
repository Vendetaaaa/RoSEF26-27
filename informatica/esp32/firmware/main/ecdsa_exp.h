#pragma once

#include <stddef.h>
#include <stdint.h>
#include "mbedtls/bignum.h"

#define ROSEF_HEX_BYTES 32

/* Experimental ECDSA signer for controlled side-channel measurements.
 * This implementation is intentionally separate from production crypto code.
 */
int rosef_ecdsa_sign_fixed_k(const uint8_t private_key[ROSEF_HEX_BYTES],
                             const uint8_t nonce_k[ROSEF_HEX_BYTES],
                             const uint8_t digest[ROSEF_HEX_BYTES],
                             uint8_t r[ROSEF_HEX_BYTES],
                             uint8_t s[ROSEF_HEX_BYTES]);

int rosef_hex_to_bytes(const char *hex, uint8_t *out, size_t out_len);
void rosef_bytes_to_hex(const uint8_t *in, size_t len, char *out, size_t out_len);
