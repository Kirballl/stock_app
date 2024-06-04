#include "core.hpp"

Serialize::TradeResponse Core::handle_order(const Serialize::TradeOrder& order) {
    Serialize::TradeResponse response;

    std::cout << "handle_order()" << std::endl;

    //save_order_to_db();
    switch (order.type()) {
    case Serialize::TradeOrder::SELL :
        sell_orders_.push_back(order);
        response.set_message(Serialize::TradeResponse::SUCCESS);
        break;
    case Serialize::TradeOrder::BUY :
        buy_orders_.push_back(order);
        response.set_message(Serialize::TradeResponse::SUCCESS);
        break;
    default:
        response.set_message(Serialize::TradeResponse::ERROR);
        break;
    }
    
    //match_orders();

    return response;
}
