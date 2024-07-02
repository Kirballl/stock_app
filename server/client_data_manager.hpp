#ifndef CLIENT_DATA_MANAGER_HPP
#define CLIENT_DATA_MANAGER_HPP

#include <string>
#include <unordered_map>
#include <shared_mutex>
#include <algorithm>


#include "common.hpp"
#include "time_order_utils.hpp"
#include "order_queue.hpp"
#include "session_manager.hpp"
#include "trade_market_protocol.pb.h"

#define AMOUNT_LAST_COMPLETED_OREDRS 100
#define AMOUNT_QUOTE_HISTORY 1000

//*INFO: Forward declaration
class SessionManager;


class ClientDataManager {
public:
    ClientDataManager(std::shared_ptr<SessionManager> session_manager);

    void initialize_from_database();

    //*INFO: Core operations
    bool update_active_order_usd_amount(const Serialize::TradeOrder& order, int32_t transaction_amount);
    bool change_client_balances_according_match(const std::string& client_sell, const std::string& client_buy,
                               int32_t transaction_amount, double transaction_cost);
    bool change_client_balance(const std::string& client_username, change_balance_type_t change_balance_type,
                               wallet_type_t wallet_type, double amount);
    bool update_clients_balances_in_db(const std::string& client_sell, const std::string& client_buy);
    bool add_order_to_completed(const Serialize::TradeOrder& completed_order);
    bool remove_order_from_active_orders(int64_t order_id, Serialize::TradeOrder::TradeType type);

    //*INFO: SessionClientConnection operations
    void create_new_client_fund_data(std::string new_key);
    void push_order_to_active_orders(const Serialize::TradeOrder& order);

    Serialize::AccountBalance get_client_balance(const std::string& client_username) const;
    Serialize::ActiveOrders get_all_active_oreders();
    Serialize::CompletedOredrs get_last_completed_oreders();
    Serialize::QuoteHistory get_quote_history();

    //*INFO: Condition variables notifications
    void notify_order_received();
    void notify_to_stop_matching_orders();
    //*INFO: Avalible only on core thread
    void stock_loop_wait_for_orders(std::shared_ptr<SessionManager> session_manager);

    //*INFO: Orders queue operations
    bool is_empty_order_queue(trade_type_t trade_type);
    bool push_order_to_order_queue(trade_type_t trade_type, const Serialize::TradeOrder& order);
    bool pop_order_from_order_queue(trade_type_t trade_type, Serialize::TradeOrder& order);

private:
    mutable std::shared_mutex client_data_mutex_;

    std::unordered_map<std::string, Serialize::AccountBalance> clients_funds_data_; 
    std::unordered_map<int64_t, Serialize::TradeOrder> active_buy_orders_;
    std::unordered_map<int64_t, Serialize::TradeOrder> active_sell_orders_;
    std::deque<Serialize::TradeOrder> completed_orders_;
    std::deque<Serialize::Quote> quote_history_;

    std::shared_ptr<SessionManager> session_manager_;

    std::condition_variable order_queue_cv_;
    std::mutex order_queue_cv_mutex_;
    std::shared_ptr<OrderQueue> buy_orders_queue_;
    std::shared_ptr<OrderQueue> sell_orders_queue_;
};

#endif // CLIENT_DATA_MANAGER_HPP
