#include "order_id_generator.hpp"

int64_t OrderIdGenerator::generate_id() {
    static std::atomic<int32_t> counter(0);
    auto now = std::chrono::system_clock::now();
    auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    int32_t count = counter.fetch_add(1, std::memory_order_relaxed);
    
    return (millis << 32) | (count & 0xFFFFFFFF);
}