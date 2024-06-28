#ifndef DATABASE_HPP
#define DATABASE_HPP

#include <string>

#include <pqxx/pqxx>
#include <spdlog/spdlog.h>
#include "bcrypt.h"

#include "trade_market_protocol.pb.h"

class Database {
public:
    Database(const std::string& connection_info);

    //*INFO auth operation
    bool is_user_exists(const std::string& username);
    void add_user(const std::string& username, const std::string& password);
    bool authenticate_user(const std::string& username, const std::string& password);

    //*INFO
    void save_active_order(const Serialize::TradeOrder& order);
    std::vector<Serialize::TradeOrder> load_active_orders(Serialize::TradeOrder::TradeType type);

private:
    pqxx::connection connection_;
};

#endif // DATABASE_HPP
