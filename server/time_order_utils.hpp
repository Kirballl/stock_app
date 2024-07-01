#pragma once

#include <cstdint>
#include <atomic>
#include <chrono>

class TimeOrderUtils {
public:
    static int64_t generate_id();
    static int64_t get_current_timestamp();
};
