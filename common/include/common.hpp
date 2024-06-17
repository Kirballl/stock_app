#ifndef CLIENSERVERECN_COMMON_HPP
#define CLIENSERVERECN_COMMON_HPP

#include <spdlog/spdlog.h>

#define LOGS_FILE_SIZE 524285 // ~5 MB
#define AMOUNT_OF_ARCHIVED_FILES 5

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

#endif //CLIENSERVERECN_COMMON_HPP
