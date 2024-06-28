#pragma once

#include <cstdint>
#include <atomic>
#include <chrono>

class OrderIdGenerator {
public:
    static int64_t generate_id();
};
