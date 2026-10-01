#include "ecdsa_exp.h"

#include <cstring>

#include "mbedtls/bignum.h"
#include "mbedtls/ecp.h"

namespace rosef {

namespace {

int hexValue(char c) {
  if (c >= '0' && c <= '9') {
    return c - '0';
  }

  if (c >= 'a' && c <= 'f') {
    return c - 'a' + 10;
  }

  if (c >= 'A' && c <= 'F') {
    return c - 'A' + 10;
  }

  return -1;
}

bool scalarInRange(
    const mbedtls_mpi *value,
    const mbedtls_mpi *order) {
  if (mbedtls_mpi_cmp_int(value, 0) <= 0) {
    return false;
  }

  return mbedtls_mpi_cmp_mpi(value, order) < 0;
}

}

int ecdsa_sign_fixed_k(
    const uint8_t private_key[32],
    const uint8_t nonce[32],
    const uint8_t digest[32],
    uint8_t r_out[32],
    uint8_t s_out[32]) {

  int rc = -1;

  mbedtls_ecp_group group;
  mbedtls_ecp_point point;

  mbedtls_mpi d;
  mbedtls_mpi k;
  mbedtls_mpi z;
  mbedtls_mpi r;
  mbedtls_mpi s;
  mbedtls_mpi x;

  mbedtls_mpi_init(&d);
  mbedtls_mpi_init(&k);
  mbedtls_mpi_init(&z);
  mbedtls_mpi_init(&r);
  mbedtls_mpi_init(&s);
  mbedtls_mpi_init(&x);

  mbedtls_ecp_group_init(&group);
  mbedtls_ecp_point_init(&point);

  do {
    if (mbedtls_ecp_group_load(
            &group,
            MBEDTLS_ECP_DP_SECP256K1) != 0) {
      break;
    }

    if (mbedtls_mpi_read_binary(&d, private_key, 32) != 0) {
      break;
    }

    if (mbedtls_mpi_read_binary(&k, nonce, 32) != 0) {
      break;
    }

    if (mbedtls_mpi_read_binary(&z, digest, 32) != 0) {
      break;
    }

    /*
     *   1 <= d < n
     *   1 <= k < n
     */
    if (!scalarInRange(&d, &group.N)) {
      break;
    }

    if (!scalarInRange(&k, &group.N)) {
      break;
    }

    /*
     * R = kG
     */
    if (mbedtls_ecp_mul(
            &group,
            &point,
            &k,
            &group.G,
            nullptr,
            nullptr) != 0) {
      break;
    }

    uint8_t point_binary[65];
    size_t point_length = 0;

    if (mbedtls_ecp_point_write_binary(
            &group,
            &point,
            MBEDTLS_ECP_PF_UNCOMPRESSED,
            &point_length,
            point_binary,
            sizeof(point_binary)) != 0) {
      break;
    }

    if (point_length != 65 || point_binary[0] != 0x04) {
      break;
    }

    /*
     *
     *   r = X mod n
     */
    if (mbedtls_mpi_read_binary(&x, &point_binary[1], 32) != 0) {
      break;
    }

    if (mbedtls_mpi_mod_mpi(&r, &x, &group.N) != 0) {
      break;
    }

    if (mbedtls_mpi_cmp_int(&r, 0) == 0) {
      break;
    }

    /*
     *   s = k^(-1) * (z + r*d) mod n
     */
    mbedtls_mpi rd;
    mbedtls_mpi numerator;
    mbedtls_mpi k_inv;

    mbedtls_mpi_init(&rd);
    mbedtls_mpi_init(&numerator);
    mbedtls_mpi_init(&k_inv);

    bool success = true;

    if (mbedtls_mpi_mul_mpi(&rd, &r, &d) != 0) {
      success = false;
    }

    if (success &&
        mbedtls_mpi_add_mpi(&numerator, &rd, &z) != 0) {
      success = false;
    }

    if (success &&
        mbedtls_mpi_mod_mpi(&numerator, &numerator, &group.N) != 0) {
      success = false;
    }

    if (success &&
        mbedtls_mpi_inv_mod(&k_inv, &k, &group.N) != 0) {
      success = false;
    }

    if (success &&
        mbedtls_mpi_mul_mpi(&s, &numerator, &k_inv) != 0) {
      success = false;
    }

    if (success &&
        mbedtls_mpi_mod_mpi(&s, &s, &group.N) != 0) {
      success = false;
    }

    if (success && mbedtls_mpi_cmp_int(&s, 0) == 0) {
      success = false;
    }

    mbedtls_mpi_free(&k_inv);
    mbedtls_mpi_free(&numerator);
    mbedtls_mpi_free(&rd);

    if (!success) {
      break;
    }
    if (mbedtls_mpi_write_binary(&r, r_out, 32) != 0) {
      break;
    }

    if (mbedtls_mpi_write_binary(&s, s_out, 32) != 0) {
      break;
    }

    rc = 0;

  } while (false);

  mbedtls_ecp_point_free(&point);
  mbedtls_ecp_group_free(&group);

  mbedtls_mpi_free(&x);
  mbedtls_mpi_free(&s);
  mbedtls_mpi_free(&r);
  mbedtls_mpi_free(&z);
  mbedtls_mpi_free(&k);
  mbedtls_mpi_free(&d);

  return rc;
}

void bytesToHex(
    const uint8_t *data,
    size_t length,
    char *output,
    size_t output_size) {

  static const char HEX_DIGITS[] = "0123456789abcdef";

  if (output == nullptr || output_size == 0) {
    return;
  }

  if (data == nullptr ||
      output_size < (length * 2 + 1)) {
    output[0] = '\0';
    return;
  }

  for (size_t i = 0; i < length; ++i) {
    output[2 * i] =
        HEX_DIGITS[(data[i] >> 4) & 0x0F];

    output[2 * i + 1] =
        HEX_DIGITS[data[i] & 0x0F];
  }

  output[length * 2] = '\0';
}

bool hexToBytes(
    const char *hex,
    uint8_t *output,
    size_t output_length) {

  if (hex == nullptr || output == nullptr) {
    return false;
  }

  const size_t expected_length = output_length * 2;

  if (strlen(hex) != expected_length) {
    return false;
  }

  for (size_t i = 0; i < output_length; ++i) {
    const int high = hexValue(hex[2 * i]);
    const int low = hexValue(hex[2 * i + 1]);

    if (high < 0 || low < 0) {
      return false;
    }

    output[i] =
        static_cast<uint8_t>((high << 4) | low);
  }

  return true;
}

}
