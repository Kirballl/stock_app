#include "core.hpp"
#include "common.hpp"

Serialize::TradeResponse Core::handle_order(const Serialize::TradeOrder& order) {
    Serialize::TradeResponse response;

    //save_order_to_db();
    // switch (order.type()) {
    // case Serialize::TradeOrder::SELL :
    //     sell_orders_.push_back(order);
    //     response.set_response_msg(Serialize::TradeResponse::SUCCESS);
    //     break;
    // case Serialize::TradeOrder::BUY :
    //     buy_orders_.push_back(order);
    //     response.set_response_msg(Serialize::TradeResponse::SUCCESS);
    //     break;
    // default:
    //     response.set_response_msg(Serialize::TradeResponse::ERROR);
    //     break;
    // }
    
    

    //match_orders();

    return response;
}
