#include "core.hpp"

Core::Core(std::shared_ptr<SessionManager> session_manager) : session_manager_(session_manager) {
}

void Core::save_all_active_orders_to_db() {
    auto database = session_manager_->get_database();

    while (!buy_orders_book_.empty()) {
        database->save_active_order_to_db(buy_orders_book_.top());
        buy_orders_book_.pop();
    }

    while (!sell_orders_book_.empty()) {
        database->save_active_order_to_db(sell_orders_book_.top());
        sell_orders_book_.pop();        
    }   
}

void Core::load_all_active_orders_from_db() {
    auto database = session_manager_->get_database();

    auto active_buy_orders = database->load_active_orders_from_db(Serialize::TradeOrder::BUY);
    for (const auto& order : active_buy_orders) {
        buy_orders_book_.push(order);
    }

    auto active_sell_orders = database->load_active_orders_from_db(Serialize::TradeOrder::SELL);
    for (const auto& order : active_sell_orders) {
        sell_orders_book_.push(order);
    }
}

//*INFO matching orders thread
void Core::stock_loop() {

    while (session_manager_->is_runnig()) {

        auto client_data_manager = session_manager_->get_client_data_manager();
        client_data_manager->stock_loop_wait_for_orders(session_manager_);

        std::lock_guard<std::mutex> match_engine_lock_guard(core_mutex_);

        complement_order_books();

        if (!session_manager_->is_runnig()) { 
            save_all_active_orders_to_db();
            break;
        }

        process_orders();
    }
}

void Core::complement_order_books() {
    auto client_data_manager = session_manager_->get_client_data_manager();

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
                move_order_to_completed_orders(buy_order);
            }
            if (sell_order.usd_amount() == 0) {
                move_order_to_completed_orders(sell_order);
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

    int32_t transaction_amount = std::min(sell_order.usd_amount(), buy_order.usd_amount());
    double transaction_cost = transaction_amount * buy_order.usd_cost(); //*INFO: RUB

    sell_order.set_usd_amount(sell_order.usd_amount() - transaction_amount);
    buy_order.set_usd_amount(buy_order.usd_amount() - transaction_amount);

    if (!update_active_order_usd_amount_in_client_data_manager(sell_order, buy_order, transaction_amount, transaction_cost)) {
        spdlog::error("Error to update active order usd_amount: BUY {}, id={} . SELL {}, id={} . Amount: {} Cost: {}",
                            buy_order.username(), buy_order.order_id(), sell_order.username(), sell_order.order_id(),
                            transaction_amount, transaction_cost);
        return false;
    }

    if (!change_clients_balances(sell_order, buy_order, transaction_amount, transaction_cost)) {
        spdlog::error("Error to change clients balances: BUY {} SELL {} - Amount: {} Cost: {}",
                        buy_order.username(), sell_order.username(), transaction_amount, transaction_cost);
        return false;
    }

    spdlog::info("Matched orders: BUY {} SELL {} - Amount: {} Cost: {}",
                        buy_order.username(), sell_order.username(), transaction_amount, transaction_cost);
    return true;
}

bool Core::update_active_order_usd_amount_in_client_data_manager (const Serialize::TradeOrder& sell_order, const Serialize::TradeOrder& buy_order,
                                int32_t transaction_amount, double transaction_cost) {
    auto client_data_manager = session_manager_->get_client_data_manager();

    if (!client_data_manager->update_active_order_usd_amount(sell_order, transaction_amount) ||
        !client_data_manager->update_active_order_usd_amount(buy_order, transaction_amount)) {
        return false; 
    }

    return true;
}

bool Core::change_clients_balances(Serialize::TradeOrder& sell_order, Serialize::TradeOrder& buy_order,
                                   int32_t transaction_amount, double transaction_cost) {

    auto client_data_manager = session_manager_->get_client_data_manager();
    return client_data_manager->change_client_balances_according_match(sell_order.username(), buy_order.username(),
                                                                            transaction_amount, transaction_cost);
}

bool Core::move_order_to_completed_orders(Serialize::TradeOrder& completed_order) {
    auto client_data_manager = session_manager_->get_client_data_manager();
    return client_data_manager->add_order_to_completed(completed_order);
}

bool Core::remove_order_by_id(int64_t order_id, trade_type_t trade_type) {
    std::lock_guard<std::mutex> remove_order_by_id_lock_guard(core_mutex_);

    if (trade_type == BUY) {
        return remove_from_queue(buy_orders_book_, order_id);
    } else
    if (trade_type == SELL) {
        return remove_from_queue(sell_orders_book_, order_id);
    } else {
        return false;
    }
}

template<typename T, typename Container, typename Compare>
bool Core::remove_from_queue(std::priority_queue<T, Container, Compare>& queue, int64_t order_id) {

    std::priority_queue<T, Container, Compare> temp_queue;
    bool found = false;

    while (!queue.empty()) {
        const auto& order = queue.top();
        if (order.order_id() != order_id) {
            temp_queue.push(std::move(const_cast<T&>(order)));
        } else {
            found = true;
        }
        queue.pop();
    }

    queue = std::move(temp_queue);
    return found;
}
