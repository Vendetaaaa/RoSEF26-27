#pragma once

#include <Arduino.h>

namespace rosef {

struct BatchConfig {
  uint32_t sample_count;
  const char *message;
};

struct BatchSample {
  uint32_t sample_id;
  uint8_t nonce[32];
  uint8_t digest[32];
  uint8_t r[32];
  uint8_t s[32];
  uint32_t elapsed_us;
};

bool batchRun(
    const BatchConfig &config);

}
