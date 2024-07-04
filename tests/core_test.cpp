#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "mock_database.hpp"
#include "session_manager.hpp"
#include "client_data_manager.hpp"
#include "core.hpp"
#include "time_order_utils.hpp"

using ::testing::_;
using ::testing::Return;

class CoreTest : public ::testing::Test {
protected:
    void SetUp() override {
        mock_database_ = std::make_shared<MockDatabase>();
        session_manager_ = std::make_shared<SessionManager>();
        session_manager_->init_mockdb(mock_database_);

        EXPECT_CALL(*mock_database_, load_active_orders_from_db(::testing::_))
            .WillRepeatedly(Return(std::vector<Serialize::TradeOrder>()));
        EXPECT_CALL(*mock_database_, load_clients_balances_from_db())
            .WillRepeatedly(Return(std::vector<Serialize::ClientBalance>()));
        EXPECT_CALL(*mock_database_, load_last_completed_orders(::testing::_))
            .WillRepeatedly(Return(std::vector<Serialize::TradeOrder>()));
        EXPECT_CALL(*mock_database_, load_quote_history(::testing::_))
            .WillRepeatedly(Return(std::vector<Serialize::Quote>()));

        session_manager_->init_core();
        session_manager_->init_client_data_manager();

        client_data_manager_ = session_manager_->get_client_data_manager();
        core_ = session_manager_->get_core();
    }

    void TearDown() override {
        client_data_manager_.reset();
        core_.reset();
        session_manager_.reset();

        testing::Mock::AllowLeak(mock_database_.get());
        mock_database_.reset();
    }

    Serialize::TradeOrder create_test_order(Serialize::TradeOrder::TradeType type, double usd_cost, int usd_amount, const std::string& username) {
        Serialize::TradeOrder order;
        order.set_type(type);
        order.set_order_id(TimeOrderUtils::generate_id());
        order.set_timestamp(TimeOrderUtils::get_current_timestamp());
        order.set_usd_cost(usd_cost);
        order.set_usd_amount(usd_amount);
        order.set_username(username);
        return order;
    }

    void add_order_to_containers(const Serialize::TradeOrder& order) {
        client_data_manager_->create_new_client_fund_data(order.username());
        client_data_manager_->push_order_to_active_orders(order);
        core_->place_order_to_priority_queue(order);
    }

    void verify_client_balance(const std::string& username, double expected_usd, double expectred_rub) {
        auto balance = client_data_manager_->get_client_balance(username);
        EXPECT_NEAR(balance.usd_balance(), expected_usd, EPSILON);
        EXPECT_NEAR(balance.rub_balance(), expectred_rub, EPSILON);
    }

    std::shared_ptr<MockDatabase> mock_database_;
    std::shared_ptr<SessionManager> session_manager_;
    std::shared_ptr<ClientDataManager> client_data_manager_;
    std::shared_ptr<Core> core_;
};

TEST_F(CoreTest, ExamlpeTestCase) {
    auto buy_order1 = create_test_order(Serialize::TradeOrder::BUY, 62.0, 10, "User1");
    auto buy_order2 = create_test_order(Serialize::TradeOrder::BUY, 63.0, 20, "User2");
    auto sell_order1 = create_test_order(Serialize::TradeOrder::SELL, 61.0, 50, "User3");

    add_order_to_containers(buy_order1);
    add_order_to_containers(buy_order2);
    add_order_to_containers(sell_order1);

    core_->process_orders();

    auto client_data_manager = session_manager_->get_client_data_manager();

    verify_client_balance("User1", 10, -620);
    verify_client_balance("User2", 20, -1260);
    verify_client_balance("User3", -30, 1880);

    auto active_orders = client_data_manager->get_all_active_oreders();
    EXPECT_EQ(active_orders.active_sell_orders_size(), 1);
    EXPECT_EQ(active_orders.active_sell_orders(0).usd_amount(), 20);
}

TEST_F(CoreTest, NonIntersectingOrders) {
    auto buy_order = create_test_order(Serialize::TradeOrder::BUY, 60.0, 10, "Buyer");
    auto sell_order = create_test_order(Serialize::TradeOrder::SELL, 61.0, 10, "Seller");

    add_order_to_containers(buy_order);
    add_order_to_containers(sell_order);

    core_->process_orders();

    verify_client_balance("Buyer", 0, 0);
    verify_client_balance("Seller", 0, 0);

    auto active_orders = client_data_manager_->get_all_active_oreders();
    EXPECT_EQ(active_orders.active_buy_orders_size(), 1);
    EXPECT_EQ(active_orders.active_sell_orders_size(), 1);
}

TEST_F(CoreTest, MatchingOrdersWithSamePriceAndVolume) {
    auto buy_order = create_test_order(Serialize::TradeOrder::BUY, 72.5, 15, "Buyer");
    auto sell_order = create_test_order(Serialize::TradeOrder::SELL, 72.5, 15, "Seller");

    add_order_to_containers(buy_order);
    add_order_to_containers(sell_order);

    core_->process_orders();

    verify_client_balance("Buyer", 15, -1087.5);
    verify_client_balance("Seller", -15, 1087.5);

    auto active_orders = client_data_manager_->get_all_active_oreders();
    EXPECT_EQ(active_orders.active_buy_orders_size(), 0);
    EXPECT_EQ(active_orders.active_sell_orders_size(), 0);
}


TEST_F(CoreTest, MultipleOrderExecution) {
    auto buy_order1 = create_test_order(Serialize::TradeOrder::BUY, 65.0, 20, "Buyer1");
    auto buy_order2 = create_test_order(Serialize::TradeOrder::BUY, 64.0, 15, "Buyer2");
    auto buy_order3 = create_test_order(Serialize::TradeOrder::BUY, 63.0, 10, "Buyer3");
    auto sell_order1 = create_test_order(Serialize::TradeOrder::SELL, 62.0, 30, "Seller1");
    auto sell_order2 = create_test_order(Serialize::TradeOrder::SELL, 63.5, 25, "Seller2");

    add_order_to_containers(buy_order1);
    add_order_to_containers(buy_order2);
    add_order_to_containers(buy_order3);
    add_order_to_containers(sell_order1);
    add_order_to_containers(sell_order2);
    
    core_->process_orders();

    verify_client_balance("Buyer1", 20, -1300);
    verify_client_balance("Buyer2", 15, -960);
    verify_client_balance("Buyer3", 0, 0);
    verify_client_balance("Seller1", -30, 1940);
    verify_client_balance("Seller2", -5, 320);

    auto active_orders = client_data_manager_->get_all_active_oreders();
    EXPECT_EQ(active_orders.active_buy_orders_size(), 1);
    EXPECT_EQ(active_orders.active_sell_orders_size(), 1);
}
