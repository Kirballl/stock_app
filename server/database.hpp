#ifndef DATABASE_HPP
#define DATABASE_HPP

#include <string>
#include <mutex>
#include <cstdint> 
#include <vector>

#include "trade_market_protocol.pb.h"

#include <pqxx/pqxx>
#include <spdlog/spdlog.h>
#include "bcrypt.h"

class IDatabase {
public:
    virtual ~IDatabase() = default;

    virtual bool is_user_exists(const std::string& username) = 0;
    virtual void add_user(const std::string& username, const std::string& password) = 0;
    virtual bool authenticate_user(const std::string& username, const std::string& password) = 0;

    virtual void save_active_order_to_db(const Serialize::TradeOrder& order) = 0;
    virtual std::vector<Serialize::TradeOrder> load_active_orders_from_db(Serialize::TradeOrder::TradeType type) = 0;

    virtual void update_actual_client_balance_in_db(const Serialize::ClientBalance& client_balance) = 0;
    virtual std::vector<Serialize::ClientBalance> load_clients_balances_from_db() = 0;

    virtual void save_completed_order_to_db(const Serialize::TradeOrder& order, int64_t completion_timestamp) = 0;
    
    virtual std::vector<Serialize::TradeOrder> load_last_completed_orders(int number) = 0;

    virtual void save_qoute_to_db(const Serialize::Quote& qoute) = 0;
    virtual std::vector<Serialize::Quote> load_quote_history(int number) = 0;

    virtual void truncate_active_orders_table() = 0;
};

class Database : public IDatabase {
public:
    Database(const std::string& connection_info);

    //*INFO auth operation
    bool is_user_exists(const std::string& username) override;
    void add_user(const std::string& username, const std::string& password) override;
    bool authenticate_user(const std::string& username, const std::string& password) override;

    //*INFO core operations
    void save_active_order_to_db(const Serialize::TradeOrder& order) override;
    std::vector<Serialize::TradeOrder> load_active_orders_from_db(Serialize::TradeOrder::TradeType type) override;

    //*INFO  client_data_manager operations
    void update_actual_client_balance_in_db(const Serialize::ClientBalance& client_balance) override;
    std::vector<Serialize::ClientBalance> load_clients_balances_from_db() override;

    void save_completed_order_to_db(const Serialize::TradeOrder& order, int64_t completion_timestamp) override;
    std::vector<Serialize::TradeOrder> load_last_completed_orders(int number) override;

    void save_qoute_to_db(const Serialize::Quote& qoute) override;
    std::vector<Serialize::Quote> load_quote_history(int number) override;

    void truncate_active_orders_table() override;

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
