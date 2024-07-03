#ifndef IDATABASE_HPP
#define IDATABASE_HPP

#include <cstdint> 
#include <string>
#include <vector>

#include "trade_market_protocol.pb.h"

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

#endif // IDATABASE_HPP