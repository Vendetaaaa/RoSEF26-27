#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "mbedtls/sha256.h"
#include "ecdsa_exp.h"

#define TAG "ROSEF"
#define TRIGGER_GPIO 4
#define MAX_LINE 320

/* Research/demo firmware only. Do not use fixed nonces or test keys in production. */
static uint8_t g_private_key[32] = {
    0x1c,0x3a,0x7b,0x2e,0x44,0x11,0x90,0xaa,0x12,0x33,0x45,0x67,0x89,0xab,0xcd,0xef,
    0x10,0x22,0x34,0x56,0x78,0x9a,0xbc,0xde,0xf0,0x13,0x57,0x9b,0xdf,0x24,0x68,0xac
};
static uint8_t g_nonce[32] = {
    0x2a,0x19,0x83,0x71,0x55,0x44,0x32,0x10,0xfe,0xdc,0xba,0x98,0x76,0x54,0x32,0x10,
    0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,0x99,0xaa,0xbb,0xcc,0xdd,0xee,0xff,0x01
};

static void trigger_set(int high) { gpio_set_level(TRIGGER_GPIO, high ? 1 : 0); }

static void print_help(void) {
    printf("OK COMMANDS\n");
    printf("PING\n");
    printf("INFO\n");
    printf("SETK <64hex>\n");
    printf("SETD <64hex>\n");
    printf("SIGN <ascii-message>\n");
    printf("SIGNHEX <64hex-digest>\n");
    printf("\n");
}

static void cmd_sign_digest(const uint8_t digest[32]) {
    uint8_t r[32], s[32];
    char rh[65], sh[65];
    trigger_set(1);
    int rc = rosef_ecdsa_sign_fixed_k(g_private_key, g_nonce, digest, r, s);
    trigger_set(0);
    if (rc != 0) {
        printf("ERR SIGN_FAILED\n");
        return;
    }
    rosef_bytes_to_hex(r, 32, rh, sizeof(rh));
    rosef_bytes_to_hex(s, 32, sh, sizeof(sh));
    printf("OK R=%s S=%s\n", rh, sh);
}

static void handle_line(char *line) {
    line[strcspn(line, "\r\n")] = 0;
    if (strcmp(line, "PING") == 0) {
        printf("OK PONG\n");
    } else if (strcmp(line, "INFO") == 0) {
        printf("OK ROSEF_ECDSA_FIXED_K SECP256K1 TRIGGER_GPIO=%d\n", TRIGGER_GPIO);
    } else if (strcmp(line, "HELP") == 0) {
        print_help();
    } else if (strncmp(line, "SETK ", 5) == 0) {
        if (rosef_hex_to_bytes(line + 5, g_nonce, 32) == 0) printf("OK K_SET\n");
        else printf("ERR BAD_K\n");
    } else if (strncmp(line, "SETD ", 5) == 0) {
        if (rosef_hex_to_bytes(line + 5, g_private_key, 32) == 0) printf("OK D_SET\n");
        else printf("ERR BAD_D\n");
    } else if (strncmp(line, "SIGNHEX ", 8) == 0) {
        uint8_t digest[32];
        if (rosef_hex_to_bytes(line + 8, digest, 32) == 0) cmd_sign_digest(digest);
        else printf("ERR BAD_DIGEST\n");
    } else if (strncmp(line, "SIGN ", 5) == 0) {
        uint8_t digest[32];
        mbedtls_sha256((const unsigned char *)(line + 5), strlen(line + 5), digest, 0);
        cmd_sign_digest(digest);
    } else if (*line) {
        printf("ERR UNKNOWN_COMMAND\n");
    }
    fflush(stdout);
}

void app_main(void) {
    gpio_config_t io = {
        .pin_bit_mask = 1ULL << TRIGGER_GPIO,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&io));
    trigger_set(0);

    ESP_LOGI(TAG, "ROSEF experimental ECDSA firmware ready");
    printf("READY ROSEF_ECDSA_FIXED_K\n");
    print_help();

    char line[MAX_LINE];
    size_t pos = 0;
    while (1) {
        int c = getchar();
        if (c == EOF) {
            vTaskDelay(pdMS_TO_TICKS(10));
            continue;
        }
        if (c == '\n' || c == '\r') {
            if (pos > 0) {
                line[pos] = 0;
                handle_line(line);
                pos = 0;
            }
        } else if (pos + 1 < sizeof(line)) {
            line[pos++] = (char)c;
        } else {
            pos = 0;
            printf("ERR LINE_TOO_LONG\n");
        }
    }
}
