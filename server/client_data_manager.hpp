#ifndef CLIENT_DATA_MANAGER_HPP
#define CLIENT_DATA_MANAGER_HPP

#include <string>
#include <unordered_map>
#include <shared_mutex>
#include <algorithm>

#include "common.hpp"
#include "order_queue.hpp"
#include "session_manager.hpp"
#include "trade_market_protocol.pb.h"

//*INFO: Forward declaration
class SessionManager;

struct ClientData {
    std::string username;
    double usd_balance;
    double rub_balance;
    std::vector<Serialize::TradeOrder> active_orders;
    std::vector<Serialize::TradeOrder> completed_orders;
};

class ClientDataManager {
public:
    ClientDataManager();

    //*INFO: Core operations
    Serialize::AccountBalance get_client_balance(std::string client_username) const;
    bool change_client_balances_according_match(std::string client_sell, std::string client_buy,
                               int32_t transaction_amount, double transaction_cost);
    bool change_client_balance(std::string client_username, change_balance_type_t change_balance_type,
                               wallet_type_t wallet_type, double amount);
    bool move_order_from_active_to_completed(std::string client_username, int64_t timestamp);

    //*INFO: SessionClientConnection operations
    bool has_username_in_client_data(std::string key);
    void create_new_client_data(std::string new_key);
    bool push_order_to_active_orders(std::string client_username, Serialize::TradeOrder& order);
    Serialize::ActiveOrders get_all_active_oreders();
    Serialize::CompletedOredrs get_all_completed_oreders();

    //*INFO: Avalible only on core thread
    void stock_loop_wait_for_orders(std::shared_ptr<SessionManager> session_manager);

    //*INFO: Condition variables notifications
    void notify_order_received();
    void notify_to_stop_matching_orders();

    //*INFO: Order queue operations
    bool is_empty_order_queue(trade_type_t trade_type);
    bool push_order_to_order_queue(trade_type_t trade_type, const Serialize::TradeOrder& order);
    bool pop_order_from_order_queue(trade_type_t trade_type, Serialize::TradeOrder& order);

private:
    mutable std::shared_mutex client_data_mutex_;
    std::unordered_map<std::string, ClientData> client_data_; 

    std::condition_variable order_queue_cv_;
    std::mutex order_queue_cv_mutex_;
    std::shared_ptr<OrderQueue> buy_orders_queue_;
    std::shared_ptr<OrderQueue> sell_orders_queue_;
};

#endif // CLIENT_DATA_MANAGER_HPP
