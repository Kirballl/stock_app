#include "core.hpp"

Serialize::TradeResponse Core::handle_order(const Serialize::TradeOrder& order) {
    Serialize::TradeResponse response;

    //save_order_to_db();
    switch (order.type()) {
    case Serialize::TradeOrder::SELL :
        sell_orders_.push_back(order);

        response.set_status("Success");
        response.set_message("Sell order received");
        break;
    case Serialize::TradeOrder::BUY :
        buy_orders_.push_back(order);

        response.set_status("Success");
        response.set_message("Buy order received");
        break;
    default:
        response.set_status("Error");
        response.set_message("Unknown order type");
        break;
    }
    
    //match_orders();

    return response;
}
