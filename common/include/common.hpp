#ifndef CLIENSERVERECN_COMMON_HPP
#define CLIENSERVERECN_COMMON_HPP

#define LOGS_FILE_SIZE 524285 // ~5 MB
#define AMOUNT_OF_ARCHIVED_FILES 5

#include <string>
#include <cstdint>
#include <iomanip>
#include <ctime>
#include <chrono>
#include <sstream>

#include <spdlog/spdlog.h> 

#include "config.hpp"

enum trade_type_t {
    BUY,
    SELL
};

enum wallet_type_t {
    RUB,
    USD
};

enum change_balance_type_t {
    INCREASE,
    DECREASE
};

spdlog::level::level_enum set_logging_level(const Config& config);

std::string timestamp_to_readable(int64_t timestamp);

#endif //CLIENSERVERECN_COMMON_HPP
