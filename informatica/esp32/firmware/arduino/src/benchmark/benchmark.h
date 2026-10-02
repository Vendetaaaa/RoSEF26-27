#pragma once

#include <Arduino.h>

namespace rosef {

struct BenchmarkResult {
  uint32_t count;
  uint64_t total_us;
  uint32_t average_us;
  uint32_t min_us;
  uint32_t max_us;
};

bool benchmarkRun(
    uint32_t count,
    BenchmarkResult &result);

}
