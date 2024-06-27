#include "core.hpp"

Core::Core(std::shared_ptr<SessionManager> session_manager) : session_manager_(session_manager) {
}

//*INFO matching orders thread
void Core::stock_loop() {
    while (session_manager_->is_runnig()) {

//                                       //
 std::cout << ">stock_loop_wait_for_orders" << std::endl;
//                                       //


        auto client_data_manager = session_manager_->get_client_data_manager();
        client_data_manager->stock_loop_wait_for_orders(session_manager_);
//                                       //
 std::cout << "stock_loop" << std::endl;
//                                       //

        if (!session_manager_->is_runnig()) { 
            break;
        }

        complement_order_books(client_data_manager);
        

//                                       //

        print_orders_book();
//                                       //

        process_orders();
    }
}


//DEBUG
void Core::print_orders_book() {
    std::cout << "Buy Orders Book:" << std::endl;
    std::priority_queue<Serialize::TradeOrder, std::vector<Serialize::TradeOrder>, BuyOrderComparator> temp_buy_queue = buy_orders_book_;
    while (!temp_buy_queue.empty()) {
        const Serialize::TradeOrder& order = temp_buy_queue.top();
        std::cout << "Type: " << (order.type() == Serialize::TradeOrder::BUY ? "BUY" : "SELL")
                << ", USD Cost: " << order.usd_cost()
                << ", USD Amount: " << order.usd_amount()
                << ", Timestamp: " << timestamp_to_readable(order.timestamp())
                << ", Order ID: " << order.order_id()
                << ", Username: " << order.username()
                << std::endl;
        temp_buy_queue.pop();
    }
    std::cout << "Sell Orders Book:" << std::endl;
    std::priority_queue<Serialize::TradeOrder, std::vector<Serialize::TradeOrder>, SellOrderComparator> temp_sell_queue = sell_orders_book_;
    while (!temp_sell_queue.empty()) {
        const Serialize::TradeOrder& order = temp_sell_queue.top();
        std::cout << "Type: " << (order.type() == Serialize::TradeOrder::BUY ? "BUY" : "SELL")
                << ", USD Cost: " << order.usd_cost()
                << ", USD Amount: " << order.usd_amount()
                << ", Timestamp: " <<  timestamp_to_readable(order.timestamp())
                << ", Order ID: " << order.order_id()
                << ", Username: " << order.username()
                << std::endl;
        temp_sell_queue.pop();
    }
}
std::string Core::timestamp_to_readable(int64_t timestamp) {
    auto millis = std::chrono::milliseconds(timestamp);
    auto secs = std::chrono::duration_cast<std::chrono::seconds>(millis);
    std::time_t time = secs.count();
    std::tm* tm = std::localtime(&time);

    std::stringstream ss;
    ss << std::put_time(tm, "%Y-%m-%d %H:%M:%S");
    ss << '.' << std::setw(3) << std::setfill('0') << (millis.count() % 1000);
    return ss.str();
}
//////////////////////////////////////

void Core::complement_order_books(std::shared_ptr<ClientDataManager> client_data_manager) {
    while(!client_data_manager->is_empty_order_queue(BUY)) {
            Serialize::TradeOrder buy_order;
            client_data_manager->pop_order_from_order_queue(BUY, buy_order);

            place_order_to_priority_queue(buy_order);
        }
        while (!client_data_manager->is_empty_order_queue(SELL)) {
            Serialize::TradeOrder sell_order;
            client_data_manager->pop_order_from_order_queue(SELL, sell_order);

            place_order_to_priority_queue(sell_order);
        }
}

void Core::place_order_to_priority_queue(const Serialize::TradeOrder& order) {
    if (order.type() == Serialize::TradeOrder::BUY) {
        buy_orders_book_.push(order);
    } else {
        sell_orders_book_.push(order);
    }
}

//*INFO: Matching engine
void Core::process_orders() {
    std::cout << "process_orders() started" << std::endl; //DEBUG

    while (!buy_orders_book_.empty() && !sell_orders_book_.empty()) {
        Serialize::TradeOrder buy_order = buy_orders_book_.top();
        buy_orders_book_.pop(); 
        Serialize::TradeOrder sell_order = sell_orders_book_.top();
        sell_orders_book_.pop();

        if (buy_order.usd_cost() > sell_order.usd_cost() || std::fabs(buy_order.usd_cost() - sell_order.usd_cost()) < EPSILON) {

            if (!match_orders(sell_order, buy_order)) {
                             spdlog::error("Error to match orders: BUY {} SELL {}",
                                            buy_order.username(), sell_order.username());
            }

            if (buy_order.usd_amount() == 0) {
                move_order_to_completed_oreders(buy_order);
            }
            if (sell_order.usd_amount() == 0) {
                move_order_to_completed_oreders(sell_order);
            }

            if (buy_order.usd_amount() > 0) {
                buy_orders_book_.push(buy_order);
            }
            if (sell_order.usd_amount() > 0) {
                sell_orders_book_.push(sell_order);
            }

        } else {
            buy_orders_book_.push(buy_order);
            sell_orders_book_.push(sell_order);

            break;
        }
    }
}

bool Core::match_orders(Serialize::TradeOrder& sell_order, Serialize::TradeOrder& buy_order) {
    //                                       //
    std::cout << "match_orders   !!! " << std::endl;
    //                                       //
    int32_t transaction_amount = std::min(sell_order.usd_amount(), buy_order.usd_amount());
    double transaction_cost = transaction_amount * buy_order.usd_cost(); //*INFO: RUB

    sell_order.set_usd_amount(sell_order.usd_amount() - transaction_amount);
    buy_order.set_usd_amount(buy_order.usd_amount() - transaction_amount);

    change_order_usd_amount_in_data_manager(sell_order, transaction_amount);
    change_order_usd_amount_in_data_manager(buy_order, transaction_amount);

    if (!change_clients_balances(sell_order, buy_order, transaction_amount, transaction_cost)) {
        spdlog::error("Error to change clients balances: BUY {} SELL {} - Amount: {} Cost: {}",
                        buy_order.username(), sell_order.username(), transaction_amount, transaction_cost);
        return false;
    }

    spdlog::info("Matched orders: BUY {} SELL {} - Amount: {} Cost: {}",
                        buy_order.username(), sell_order.username(), transaction_amount, transaction_cost);
    return true;
}

bool Core::change_clients_balances(Serialize::TradeOrder& sell_order, Serialize::TradeOrder& buy_order,
                                   int32_t transaction_amount, double transaction_cost) {

    auto client_data_manager = session_manager_->get_client_data_manager();
    return client_data_manager->change_client_balances_according_match(sell_order.username(), buy_order.username(),
                                                                            transaction_amount, transaction_cost);
}

bool Core::change_order_usd_amount_in_data_manager(Serialize::TradeOrder& order, int32_t transaction_amount) {
    auto client_data_manager = session_manager_->get_client_data_manager();
    return client_data_manager->change_order_usd_amount_according_match(order.username(), order.timestamp(), transaction_amount);
}

bool Core::move_order_to_completed_oreders(Serialize::TradeOrder& order) {
    auto client_data_manager = session_manager_->get_client_data_manager();
    return client_data_manager->move_order_from_active_to_completed(order.username(), order.timestamp());
}
