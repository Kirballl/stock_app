#ifndef DATABASE_HPP
#define DATABASE_HPP

#include <string>
#include <mutex>

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

    //*INFO core operations
    void save_active_order_to_db(const Serialize::TradeOrder& order);
    std::vector<Serialize::TradeOrder> load_active_orders_from_db(Serialize::TradeOrder::TradeType type);

    //*INFO  client_data_manager operations
    void update_actual_client_balance_in_db(const Serialize::ClientBalance& client_balance);
    std::vector<Serialize::ClientBalance> load_clients_balances_from_db();

    void save_completed_order_to_db(const Serialize::TradeOrder& order, int64_t completion_timestamp);
    std::vector<Serialize::TradeOrder> load_last_completed_orders(int number);

    void save_qoute_to_db(const Serialize::Quote& qoute);
    std::vector<Serialize::Quote> load_quote_history(int number);

    void truncate_active_orders_table();

private:
    pqxx::connection connection_;
    mutable std::mutex mutex_;

    static const char* CREATE_USERS_TABLE;
    static const char* CREATE_ACTIVE_BUY_ORDERS_TABLE;
    static const char* CREATE_ACTIVE_SELL_ORDERS_TABLE;
    static const char* CREATE_CLIENTS_BALANCES_TABLE;
    static const char* CREATE_COMPLETED_ORDERS_TABLE;
    static const char* CREATE_QUOTE_HISTORY_TABLE;
};

#endif // DATABASE_HPP
