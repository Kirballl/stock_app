#ifndef CORE_HPP
#define CORE_HPP

#include <vector>
#include <algorithm>
#include <queue>
#include <memory>
#include <string>

#include "spdlog/spdlog.h"

#include "common.hpp"
#include "session_manager.hpp"
#include "client_data_manager.hpp"
#include "session_client_connection.hpp"
#include "trade_market_protocol.pb.h"

constexpr double EPSILON = 1e-6;

// DEBUG //
#include <iomanip>
#include <ctime>
#include <chrono>
#include <sstream>

//*INFO: Forward declaration
class SessionManager;
class ClientDataManager;

struct BuyOrderComparator {
    bool operator()(const Serialize::TradeOrder& lhs, const Serialize::TradeOrder& rhs) const {
        if (std::fabs(lhs.usd_cost() - rhs.usd_cost()) < EPSILON) {
            return lhs.timestamp() > rhs.timestamp();
        }
        return lhs.usd_cost() < rhs.usd_cost();
    }
};
struct SellOrderComparator {
    bool operator()(const Serialize::TradeOrder& lhs, const Serialize::TradeOrder& rhs) const {
        if (std::fabs(lhs.usd_cost() - rhs.usd_cost()) < EPSILON) {
            return lhs.timestamp() > rhs.timestamp();
        }
        return lhs.usd_cost() > rhs.usd_cost();
    }
};

class Core : public std::enable_shared_from_this<Core> {
public:
    Core(std::shared_ptr<SessionManager> session_manager);

    void stock_loop();

    void save_all_active_orders_to_db();
    void load_all_active_orders_from_db();

private:
    void complement_order_books();
    void place_order_to_priority_queue(const Serialize::TradeOrder& order);
    void process_orders(); 
    bool match_orders(Serialize::TradeOrder& sell_order, Serialize::TradeOrder& buy_order); 
    bool update_active_order_usd_amount_in_client_data_manager (
                                const Serialize::TradeOrder& sell_order, const Serialize::TradeOrder& buy_order,
                                int32_t transaction_amount, double transaction_cost);
    bool change_clients_balances(Serialize::TradeOrder& sell_order, 
                                Serialize::TradeOrder& buy_order_iterator,
                                 int32_t transaction_amount, double transaction_cost); 
    bool move_order_to_completed_orders(Serialize::TradeOrder& completed_order);


// DEBUG //
   void print_orders_book();
   std::string timestamp_to_readable(int64_t timestamp);
// //


private:
   std::priority_queue<Serialize::TradeOrder, std::vector<Serialize::TradeOrder>, BuyOrderComparator> buy_orders_book_;
   std::priority_queue<Serialize::TradeOrder, std::vector<Serialize::TradeOrder>, SellOrderComparator> sell_orders_book_;

   std::shared_ptr<SessionManager> session_manager_;
};

#endif // CORE_HPP
