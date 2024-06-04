#ifndef CLIENSERVERECN_COMMON_HPP
#define CLIENSERVERECN_COMMON_HPP

#include <string>

static short port = 5555; // config
const std::string host = "127.0.0.1"; // config 

enum trade_type_t {
    BUY,
    SELL
};

// namespace Requests {
//     static std::string Registration = "Reg";
//     static std::string Hello = "Hel";
// }

#endif //CLIENSERVERECN_COMMON_HPP
