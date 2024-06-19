#ifndef CORE_HPP
#define CORE_HPP

#include <vector>
#include <algorithm>

#include "spdlog/spdlog.h"

#include "common.hpp"
#include "session_manager.hpp"
#include "trade_market_protocol.pb.h"

class Core : public std::enable_shared_from_this<Core> {
public:
   Core(std::shared_ptr<SessionManager> session_manager);

   void stock_loop();

private:
   //TODO    void save_order_to_db();
   void place_order_to_sorted_vector(const Serialize::TradeOrder& order);
   void process_orders(); 
   void match_orders(Serialize::TradeOrder& sell_order, 
                    std::vector<Serialize::TradeOrder>::iterator buy_order_iterator); 
   void change_clients_balances(Serialize::TradeOrder& sell_order, 
                    std::vector<Serialize::TradeOrder>::iterator buy_order_iterator,
                    int32_t transaction_amount, double transaction_cost); 
   template<typename OrderType>
   void move_order_to_completed_oreders(OrderType& order);

private:
   std::vector<Serialize::TradeOrder> buy_orders_book_;
   std::vector<Serialize::TradeOrder> sell_orders_book_;

   std::shared_ptr<SessionManager> ptr_session_manager_;
};

#endif // CORE_HPP
