#include "ecdsa_exp.h"

#include <string.h>
#include "mbedtls/ecp.h"
#include "mbedtls/sha256.h"

static const char *HEX = "0123456789abcdef";

int rosef_hex_to_bytes(const char *hex, uint8_t *out, size_t out_len) {
    if (!hex || !out || strlen(hex) != out_len * 2) return -1;
    for (size_t i = 0; i < out_len; ++i) {
        char a = hex[2 * i], b = hex[2 * i + 1];
        int hi = (a >= '0' && a <= '9') ? a - '0' : (a >= 'a' && a <= 'f') ? a - 'a' + 10 : (a >= 'A' && a <= 'F') ? a - 'A' + 10 : -1;
        int lo = (b >= '0' && b <= '9') ? b - '0' : (b >= 'a' && b <= 'f') ? b - 'a' + 10 : (b >= 'A' && b <= 'F') ? b - 'A' + 10 : -1;
        if (hi < 0 || lo < 0) return -1;
        out[i] = (uint8_t)((hi << 4) | lo);
    }
    return 0;
}

void rosef_bytes_to_hex(const uint8_t *in, size_t len, char *out, size_t out_len) {
    if (!in || !out || out_len < len * 2 + 1) return;
    for (size_t i = 0; i < len; ++i) {
        out[2*i] = HEX[in[i] >> 4];
        out[2*i+1] = HEX[in[i] & 0x0f];
    }
    out[len * 2] = '\0';
}

static int deterministic_rng(void *ctx, unsigned char *out, size_t len) {
    uint32_t *state = (uint32_t *)ctx;
    if (!state || !out) return -1;
    for (size_t i = 0; i < len; ++i) {
        *state ^= *state << 13;
        *state ^= *state >> 17;
        *state ^= *state << 5;
        out[i] = (unsigned char)(*state & 0xffu);
    }
    return 0;
}

static int mpi_from_32(const uint8_t in[32], mbedtls_mpi *x) {
    return mbedtls_mpi_read_binary(x, in, 32);
}

static int mpi_to_32(const mbedtls_mpi *x, uint8_t out[32]) {
    return mbedtls_mpi_write_binary(x, out, 32);
}

int rosef_ecdsa_sign_fixed_k(const uint8_t private_key[32],
                             const uint8_t nonce_k[32],
                             const uint8_t digest[32],
                             uint8_t r[32],
                             uint8_t s[32]) {
    int rc = -1;
    mbedtls_ecp_group grp;
    mbedtls_ecp_point R;
    mbedtls_mpi d, k, z, rr, kinv, tmp, ss;

    mbedtls_ecp_group_init(&grp);
    mbedtls_ecp_point_init(&R);
    mbedtls_mpi_init(&d); mbedtls_mpi_init(&k); mbedtls_mpi_init(&z);
    mbedtls_mpi_init(&rr); mbedtls_mpi_init(&kinv); mbedtls_mpi_init(&tmp); mbedtls_mpi_init(&ss);

    if (mbedtls_ecp_group_load(&grp, MBEDTLS_ECP_DP_SECP256K1) != 0) goto cleanup;
    if (mpi_from_32(private_key, &d) != 0 || mpi_from_32(nonce_k, &k) != 0 || mpi_from_32(digest, &z) != 0) goto cleanup;
    if (mbedtls_mpi_cmp_int(&d, 0) <= 0 || mbedtls_mpi_cmp_mpi(&d, &grp.N) >= 0) goto cleanup;
    if (mbedtls_mpi_cmp_int(&k, 0) <= 0 || mbedtls_mpi_cmp_mpi(&k, &grp.N) >= 0) goto cleanup;

    /* R = k*G; r = R.x mod n. */
    uint32_t rng_state = 0x13579bdfu;
    if (mbedtls_ecp_mul(&grp, &R, &k, &grp.G, deterministic_rng, &rng_state) != 0) goto cleanup;
    if (mbedtls_mpi_mod_mpi(&rr, &R.X, &grp.N) != 0) goto cleanup;
    if (mbedtls_mpi_cmp_int(&rr, 0) == 0) goto cleanup;

    /* s = k^-1 (z + r*d) mod n. */
    if (mbedtls_mpi_inv_mod(&kinv, &k, &grp.N) != 0) goto cleanup;
    if (mbedtls_mpi_mul_mpi(&tmp, &rr, &d) != 0) goto cleanup;
    if (mbedtls_mpi_add_mpi(&tmp, &tmp, &z) != 0) goto cleanup;
    if (mbedtls_mpi_mul_mpi(&ss, &kinv, &tmp) != 0) goto cleanup;
    if (mbedtls_mpi_mod_mpi(&ss, &ss, &grp.N) != 0) goto cleanup;
    if (mbedtls_mpi_cmp_int(&ss, 0) == 0) goto cleanup;

    if (mpi_to_32(&rr, r) != 0 || mpi_to_32(&ss, s) != 0) goto cleanup;
    rc = 0;

cleanup:
    mbedtls_ecp_group_free(&grp);
    mbedtls_ecp_point_free(&R);
    mbedtls_mpi_free(&d); mbedtls_mpi_free(&k); mbedtls_mpi_free(&z);
    mbedtls_mpi_free(&rr); mbedtls_mpi_free(&kinv); mbedtls_mpi_free(&tmp); mbedtls_mpi_free(&ss);
    return rc;
}
