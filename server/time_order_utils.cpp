#include "time_order_utils.hpp"

int64_t TimeOrderUtils::generate_id() {
    static std::atomic<int32_t> counter(0);
    auto now = std::chrono::system_clock::now();
    auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    int32_t count = counter.fetch_add(1, std::memory_order_relaxed);
    
    return (millis << 32) | (count & 0xFFFFFFFF);
}

int64_t TimeOrderUtils::get_current_timestamp() {
    auto now = std::chrono::system_clock::now();
    auto now_ms = std::chrono::time_point_cast<std::chrono::milliseconds>(now);
    auto epoch = now_ms.time_since_epoch();
    int64_t timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(epoch).count();
    return timestamp;
}