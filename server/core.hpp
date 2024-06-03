#ifndef ORDER_HANDLER_HPP
#define ORDER_HANDLER_HPP

#include <vector>

#include "proto_example.pb.h"

class Core {
 public:
  Core() = default;

  Serialize::TradeResponse handle_order(const Serialize::TradeOrder& order);
  //Serialize::TradeResponse process_oreders(const Serialize::TradeRequest& request);

 private:
  //void save_order_to_db();
  //void match_orders();

 private:
  std::vector<Serialize::TradeOrder> buy_orders_;
  std::vector<Serialize::TradeOrder> sell_orders_;
};

#endif // ORDER_HANDLER_HPP
