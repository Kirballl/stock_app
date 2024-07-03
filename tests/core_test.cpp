#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <iostream>

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

    std::shared_ptr<MockDatabase> mock_database_;
    std::shared_ptr<SessionManager> session_manager_;
    std::shared_ptr<ClientDataManager> client_data_manager_;
    std::shared_ptr<Core> core_;
};

TEST_F(CoreTest, TestOrderMatchingWhenPriceCrosses) {

    Serialize::TradeOrder buy_order1, buy_order2, sell_order1;

    buy_order1.set_type(Serialize::TradeOrder::BUY);
    buy_order1.set_order_id(TimeOrderUtils::generate_id());
    buy_order1.set_timestamp(TimeOrderUtils::get_current_timestamp());
    buy_order1.set_usd_cost(62.0);
    buy_order1.set_usd_amount(10);
    buy_order1.set_username("User1");

    buy_order2.set_type(Serialize::TradeOrder::BUY);
    buy_order2.set_order_id(TimeOrderUtils::generate_id());
    buy_order2.set_timestamp(TimeOrderUtils::get_current_timestamp());
    buy_order2.set_usd_cost(63.0);
    buy_order2.set_usd_amount(20);
    buy_order2.set_username("User2");

    sell_order1.set_type(Serialize::TradeOrder::SELL);
    sell_order1.set_order_id(TimeOrderUtils::generate_id());
    sell_order1.set_timestamp(TimeOrderUtils::get_current_timestamp());
    sell_order1.set_usd_cost(61.0);
    sell_order1.set_usd_amount(50);
    sell_order1.set_username("User3");

    client_data_manager_->create_new_client_fund_data("User1");
    client_data_manager_->create_new_client_fund_data("User2");
    client_data_manager_->create_new_client_fund_data("User3");

    client_data_manager_->push_order_to_active_orders(buy_order1);
    client_data_manager_->push_order_to_active_orders(buy_order2);
    client_data_manager_->push_order_to_active_orders(sell_order1);

    core_->place_order_to_priority_queue(buy_order1);
    core_->place_order_to_priority_queue(buy_order2);
    core_->place_order_to_priority_queue(sell_order1);

    core_->process_orders();

    auto client_data_manager = session_manager_->get_client_data_manager();

    EXPECT_NEAR(client_data_manager->get_client_balance("User1").usd_balance(),    10, EPSILON);
    EXPECT_NEAR(client_data_manager->get_client_balance("User1").rub_balance(),  -620, EPSILON);
    EXPECT_NEAR(client_data_manager->get_client_balance("User2").usd_balance(),    20, EPSILON);
    EXPECT_NEAR(client_data_manager->get_client_balance("User2").rub_balance(), -1260, EPSILON);
    EXPECT_NEAR(client_data_manager->get_client_balance("User3").usd_balance(),   -30, EPSILON);
    EXPECT_NEAR(client_data_manager->get_client_balance("User3").rub_balance(),  1880, EPSILON);

    auto active_orders = client_data_manager->get_all_active_oreders();
    EXPECT_EQ(active_orders.active_sell_orders_size(), 1);
    EXPECT_EQ(active_orders.active_sell_orders(0).usd_amount(), 20);
}

