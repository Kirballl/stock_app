#ifndef MOCK_DATABASE_HPP
#define MOCK_DATABASE_HPP

#include <cstdint>
#include <gmock/gmock.h>

#include "i_database.hpp"
#include "trade_market_protocol.pb.h"

class MockDatabase : public IDatabase {
public:
    MockDatabase() = default;
    ~MockDatabase() override = default;

    void update_actual_client_balance_in_db(const Serialize::ClientBalance& client_balance) override {return;}
    void save_completed_order_to_db(const Serialize::TradeOrder& order, int64_t completion_timestamp) override {return;}
    void save_qoute_to_db(const Serialize::Quote& quote) override {return;}

    MOCK_METHOD(std::vector<Serialize::TradeOrder>, load_active_orders_from_db, (Serialize::TradeOrder::TradeType type), (override));
    MOCK_METHOD(std::vector<Serialize::ClientBalance>, load_clients_balances_from_db, (), (override));
    MOCK_METHOD(std::vector<Serialize::TradeOrder>, load_last_completed_orders, (int number), (override));
    MOCK_METHOD(std::vector<Serialize::Quote>, load_quote_history, (int number), (override));

    MOCK_METHOD(bool, is_user_exists, (const std::string& username), (override));
    MOCK_METHOD(void, add_user, (const std::string& username, const std::string& password), (override));
    MOCK_METHOD(bool, authenticate_user, (const std::string& username, const std::string& password), (override));
    MOCK_METHOD(void, save_active_order_to_db, (const Serialize::TradeOrder& order), (override));
    MOCK_METHOD(void, truncate_active_orders_table, (), (override));

};

#endif // MOCK_DATABASE_HPP
