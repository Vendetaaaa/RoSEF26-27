#include <Arduino.h>

#include "ecdsa_exp.h"

namespace rosef {

constexpr uint32_t SERIAL_BAUD = 115200;
constexpr uint8_t TRIGGER_GPIO = 4;
constexpr size_t MAX_LINE = 256;

/*
 * Test-only values.
 */
uint8_t g_private_key[32] = {
  0x1c, 0x3a, 0x7b, 0x2e, 0x44, 0x11, 0x90, 0xaa,
  0x12, 0x33, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
  0x10, 0x22, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde,
  0xf0, 0x13, 0x57, 0x9b, 0xdf, 0x24, 0x68, 0xac
};

uint8_t g_nonce[32] = {
  0x2a, 0x19, 0x83, 0x71, 0x55, 0x44, 0x32, 0x10,
  0xfe, 0xdc, 0xba, 0x98, 0x76, 0x54, 0x32, 0x10,
  0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88,
  0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff, 0x01
};

char line[MAX_LINE];
size_t line_length = 0;

void printHelp() {
  Serial.println(F("OK COMMANDS"));
  Serial.println(F("PING"));
  Serial.println(F("INFO"));
  Serial.println(F("STATUS"));
  Serial.println(F("SETK <64hex>"));
  Serial.println(F("SETD <64hex>"));
  Serial.println(F("SIGN <ascii-message>"));
  Serial.println(F("SIGNHEX <64hex-digest>"));
  Serial.println(F("PULSE"));
  Serial.println(F("HELP"));
}

void printInfo() {
  Serial.print(F("OK ROSEF_ECDSA_FIXED_K"));
  Serial.print(F(" SECP256K1"));
  Serial.print(F(" TRIGGER_GPIO="));
  Serial.print(TRIGGER_GPIO);
  Serial.print(F(" SERIAL="));
  Serial.println(SERIAL_BAUD);
}

void printStatus() {
  Serial.print(F("OK STATUS UPTIME_MS="));
  Serial.print(millis());

  Serial.print(F(" TRIGGER="));
  Serial.print(
    digitalRead(TRIGGER_GPIO) ? F("HIGH") : F("LOW")
  );

  Serial.println();
}

void triggerPulse() {
  constexpr uint32_t TEST_TRIGGER_US = 1000;

  const uint32_t start_us = micros();

  digitalWrite(TRIGGER_GPIO, HIGH);
  delayMicroseconds(TEST_TRIGGER_US);
  digitalWrite(TRIGGER_GPIO, LOW);

  Serial.print(F("OK PULSE WIDTH_US="));
  Serial.println(micros() - start_us);
}

void signDigest(const uint8_t digest[32]) {
  uint8_t r[32];
  uint8_t s[32];

  char r_hex[65];
  char s_hex[65];

  digitalWrite(TRIGGER_GPIO, HIGH);

  const int rc = ecdsa_sign_fixed_k(
    g_private_key,
    g_nonce,
    digest,
    r,
    s
  );

  digitalWrite(TRIGGER_GPIO, LOW);

  if (rc != 0) {
    Serial.println(F("ERR SIGN_FAILED"));
    return;
  }

  bytesToHex(r, sizeof(r), r_hex, sizeof(r_hex));
  bytesToHex(s, sizeof(s), s_hex, sizeof(s_hex));

  Serial.print(F("OK R="));
  Serial.print(r_hex);

  Serial.print(F(" S="));
  Serial.println(s_hex);
}

void handleSignMessage(const char *message) {
  uint8_t digest[32];

  mbedtls_sha256_context ctx;
  mbedtls_sha256_init(&ctx);

  if (mbedtls_sha256_starts_ret(&ctx, 0) != 0 ||
      mbedtls_sha256_update_ret(
        &ctx,
        reinterpret_cast<const unsigned char *>(message),
        strlen(message)
      ) != 0 ||
      mbedtls_sha256_finish_ret(&ctx, digest) != 0) {

    mbedtls_sha256_free(&ctx);

    Serial.println(F("ERR HASH_FAILED"));
    return;
  }

  mbedtls_sha256_free(&ctx);

  signDigest(digest);
}

void handleLine(char *command) {
  if (strcmp(command, "PING") == 0) {
    Serial.println(F("OK PONG"));
    return;
  }

  if (strcmp(command, "INFO") == 0) {
    printInfo();
    return;
  }

  if (strcmp(command, "STATUS") == 0) {
    printStatus();
    return;
  }

  if (strcmp(command, "PULSE") == 0) {
    triggerPulse();
    return;
  }

  if (strcmp(command, "HELP") == 0) {
    printHelp();
    return;
  }

  if (strncmp(command, "SETK ", 5) == 0) {
    if (hexToBytes(command + 5, g_nonce, sizeof(g_nonce))) {
      Serial.println(F("OK K_SET"));
    } else {
      Serial.println(F("ERR BAD_K"));
    }
    return;
  }

  if (strncmp(command, "SETD ", 5) == 0) {
    if (hexToBytes(
          command + 5,
          g_private_key,
          sizeof(g_private_key))) {

      Serial.println(F("OK D_SET"));
    } else {
      Serial.println(F("ERR BAD_D"));
    }
    return;
  }

  if (strncmp(command, "SIGNHEX ", 8) == 0) {
    uint8_t digest[32];

    if (!hexToBytes(command + 8, digest, sizeof(digest))) {
      Serial.println(F("ERR BAD_DIGEST"));
      return;
    }

    signDigest(digest);
    return;
  }

  if (strncmp(command, "SIGN ", 5) == 0) {
    handleSignMessage(command + 5);
    return;
  }

  if (*command != '\0') {
    Serial.println(F("ERR UNKNOWN_COMMAND"));
  }
}

void readSerial() {
  while (Serial.available() > 0) {
    const int value = Serial.read();

    if (value < 0) {
      return;
    }

    const char c = static_cast<char>(value);

    if (c == '\r' || c == '\n') {
      if (line_length == 0) {
        continue;
      }

      line[line_length] = '\0';
      handleLine(line);
      line_length = 0;
      continue;
    }

    if (line_length + 1 >= MAX_LINE) {
      line_length = 0;
      Serial.println(F("ERR LINE_TOO_LONG"));
      continue;
    }

    line[line_length++] = c;
  }
}

}

void setup() {
  pinMode(rosef::TRIGGER_GPIO, OUTPUT);
  digitalWrite(rosef::TRIGGER_GPIO, LOW);

  Serial.begin(rosef::SERIAL_BAUD);

  delay(250);

  Serial.println(F("READY ROSEF_ECDSA_FIXED_K"));

  rosef::printInfo();
  rosef::printHelp();
}

void loop() {
  rosef::readSerial();
}
