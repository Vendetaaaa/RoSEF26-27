#include "benchmark.h"
#include "../experiment/experiment.h"
#include <Arduino.h> // pinMode și digitalWrite

namespace rosef {

bool benchmarkRun(
    uint32_t count,
    BenchmarkResult &result) {

  result.count = 0;
  result.total_us = 0;
  result.average_us = 0;
  result.min_us = 0;
  result.max_us = 0;

  if (count == 0) {
    return false;
  }

  uint8_t digest[32];

  if (!experimentSha256(
          "rosef-benchmark",
          digest)) {
    return false;
  }

  uint8_t *nonce =
      experimentDefaultNonce();

  uint32_t min_us = UINT32_MAX;
  uint32_t max_us = 0;
  uint64_t total_us = 0;

  for (uint32_t i = 0; i < count; ++i) {
    SignResult sign_result;

    if (!experimentSignDigest(
            digest,
            nonce,
            sign_result)) {
      return false;
    }

    const uint32_t elapsed_us =
        sign_result.elapsed_us;

    total_us += elapsed_us;

    if (elapsed_us < min_us) {
      min_us = elapsed_us;
    }

    if (elapsed_us > max_us) {
      max_us = elapsed_us;
    }
  }

  result.count = count;
  result.total_us = total_us;

  result.average_us =
      static_cast<uint32_t>(
          total_us / count);

  result.min_us = min_us;
  result.max_us = max_us;

  return true;
}

}
