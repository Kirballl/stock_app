#include "core.hpp"

Core::Core(std::shared_ptr<SessionManager> session_manager) : session_manager_(session_manager) {
}

// matching orders thread
void Core::stock_loop() {
    while (session_manager_->is_runnig()) {

        auto client_data_manager = session_manager_->get_client_data_manager();
        client_data_manager->stock_loop_wait_for_orders(session_manager_);

        if (!session_manager_->is_runnig()) { 
            break;
        }

        while(!client_data_manager->is_empty_order_queue(BUY)) {
            Serialize::TradeOrder buy_order;
            client_data_manager->pop_order_from_order_queue(BUY, buy_order);
            place_order_to_sorted_vector(buy_order);
        }
        while (!client_data_manager->is_empty_order_queue(SELL)) {
            Serialize::TradeOrder sell_order;
            client_data_manager->pop_order_from_order_queue(SELL, sell_order);
            place_order_to_sorted_vector(sell_order);
        }

        process_orders();
    }
}

void Core::place_order_to_sorted_vector(const Serialize::TradeOrder& order) {
    std::vector<Serialize::TradeOrder>& target_vector = 
         (order.type() == Serialize::TradeOrder::BUY) ? buy_orders_book_ : sell_orders_book_;

    constexpr double EPSILON = 1e-6;
    auto compare_oreders_functor = [EPSILON, &order](const Serialize::TradeOrder&placed_order, const Serialize::TradeOrder&inserting_order) {
        if (order.type() == Serialize::TradeOrder::SELL) {
            if (std::fabs(placed_order.usd_cost() - inserting_order.usd_cost()) < EPSILON) {
                return placed_order.timestamp() < inserting_order.timestamp();       
            }
            return placed_order.usd_cost() < inserting_order.usd_cost();
        }
        else {
            if (std::fabs(placed_order.usd_cost() - inserting_order.usd_cost()) < EPSILON) {
                return placed_order.timestamp() < inserting_order.timestamp();       
            }
            return placed_order.usd_cost() > inserting_order.usd_cost();
        }
    };

    auto iterator = std::lower_bound(target_vector.begin(), target_vector.end(), order, compare_oreders_functor);

    target_vector.insert(iterator, order);
}

void Core::process_orders() {
    constexpr double EPSILON = 1e-6;

    for (auto& sell_order : sell_orders_book_) {
        while (!buy_orders_book_.empty()) {
            auto buy_order_iterator = buy_orders_book_.begin();

            if (buy_order_iterator->usd_cost() > sell_order.usd_cost() || std::fabs(buy_order_iterator->usd_cost() - sell_order.usd_cost()) < EPSILON) {

                if (!match_orders(sell_order, buy_order_iterator)) {
                    spdlog::error("Error to match orders: BUY {} SELL {}",
                                    buy_order_iterator->username(), sell_order.username());
                }

                if(buy_order_iterator->usd_amount() == 0) {
                    if (!move_order_to_completed_oreders(*buy_order_iterator)) {
                        spdlog::error("Error move order to completed: BUY {}",
                                                buy_order_iterator->username());
                    }
                    
                    buy_order_iterator = buy_orders_book_.erase(buy_order_iterator);
                }

                if (sell_order.usd_amount() == 0) {
                    if (!move_order_to_completed_oreders(sell_order)) {
                        spdlog::error("Error move order to completed: SELL {}",
                                                sell_order.username());
                    }
                    break;
                }
            }
        }
    }
    
    auto iterator = sell_orders_book_.begin();
    while (iterator != sell_orders_book_.end()) {
        if (iterator->usd_amount() == 0) {
            iterator = sell_orders_book_.erase(iterator);
        } else {
            ++iterator;
        }
    }
}

bool Core::match_orders(Serialize::TradeOrder& sell_order, std::vector<Serialize::TradeOrder>::iterator buy_order_iterator) {
    std::cout << "match_orders" << std::endl;

    int32_t transaction_amount = std::min(sell_order.usd_amount(), buy_order_iterator->usd_amount());
    double transaction_cost = transaction_amount * buy_order_iterator->usd_cost(); // RUB

    // Update oreders in vector 
    sell_order.set_usd_amount(sell_order.usd_amount() - transaction_amount);
    buy_order_iterator->set_usd_amount(buy_order_iterator->usd_amount() - transaction_amount);

    if (!change_clients_balances(sell_order, buy_order_iterator, transaction_amount, transaction_cost)) {
        spdlog::error("Error to change clients balances: BUY {} SELL {} - Amount: {} Cost: {}",
                        buy_order_iterator->username(), sell_order.username(), transaction_amount, transaction_cost);
        return false;
    }

    spdlog::info("Matched orders: BUY {} SELL {} - Amount: {} Cost: {}",
                        buy_order_iterator->username(), sell_order.username(), transaction_amount, transaction_cost);
    return true;
}

bool Core::change_clients_balances(Serialize::TradeOrder& sell_order, std::vector<Serialize::TradeOrder>::iterator buy_order_iterator,
                                   int32_t transaction_amount, double transaction_cost) {

    auto client_data_manager = session_manager_->get_client_data_manager();
    return client_data_manager->change_client_balances_according_match(sell_order.username(), buy_order_iterator->username(),
                                                                            transaction_amount, transaction_cost);
}

template<typename OrderType>
bool Core::move_order_to_completed_oreders(OrderType& order) {
    auto client_data_manager = session_manager_->get_client_data_manager();
    return client_data_manager->move_order_from_active_to_completed(order.username(), order.timestamp());
}
