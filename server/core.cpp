#include "core.hpp"
#include "common.hpp"

Core::Core(std::shared_ptr<SessionManager> session_manager) : ptr_session_manager_(session_manager) {
}

// matching orders thread
void Core::stock_loop() {
    while (ptr_session_manager_->is_runnig()) {
        std::unique_lock<std::mutex> order_queue_unique_lock(ptr_session_manager_->order_queue_cv_mutex);

        // Wait order from order queue 
        ptr_session_manager_->order_queue_cv.wait(order_queue_unique_lock, [this] {
            return !ptr_session_manager_->buy_orders_queue->is_empty()  ||
                   !ptr_session_manager_->sell_orders_queue->is_empty() ||
                   !ptr_session_manager_->is_runnig();
        });

        if (!ptr_session_manager_->is_runnig()) {
            break;
        }

        while(!ptr_session_manager_->buy_orders_queue->is_empty()) {
            Serialize::TradeOrder buy_order;
            ptr_session_manager_->buy_orders_queue->pop(buy_order);
            place_order_to_sorted_vector(buy_order);
        }
        while (!ptr_session_manager_->sell_orders_queue->is_empty()) {
            Serialize::TradeOrder sell_order;
            ptr_session_manager_->sell_orders_queue->pop(sell_order);
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
                return placed_order.timestamp() > inserting_order.timestamp();       
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
        std::cout << "auto& sell_order : sell_orders_book_" << std::endl;
        while (true) {
            auto match_orders_functor = [EPSILON](const Serialize::TradeOrder&buy_order, const Serialize::TradeOrder&possible_sell_order) {
                return buy_order.usd_cost() > possible_sell_order.usd_cost() || (buy_order.usd_cost() - possible_sell_order.usd_cost()) < EPSILON;
            };

            auto buy_order_iterator = std::lower_bound(buy_orders_book_.begin(),
                                                       buy_orders_book_.end(),
                                                       sell_order, match_orders_functor);

            if (buy_order_iterator == buy_orders_book_.begin()) {
                // Order can not be matched
                std::cout << "buy_order_iterator == buy_orders_book_.begin()" << std::endl;
                break;
            }

            buy_order_iterator = std::prev(buy_order_iterator);

            match_orders(sell_order, buy_order_iterator);

            if(buy_order_iterator->usd_amount() == 0) {
                buy_order_iterator = buy_orders_book_.erase(buy_order_iterator);
            }

            if (sell_order.usd_amount() == 0) {
                break;
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

void Core::match_orders(Serialize::TradeOrder& sell_order, std::vector<Serialize::TradeOrder>::iterator buy_order_iterator) {
    std::cout << "match_orders" << std::endl;
    int32_t transaction_amount = std::min(sell_order.usd_amount(), buy_order_iterator->usd_amount());
    double transaction_cost = transaction_amount * sell_order.usd_cost(); // RUB

    // Update oreders in vector 
    sell_order.set_usd_amount(sell_order.usd_amount() - transaction_amount);
    buy_order_iterator->set_usd_amount(buy_order_iterator->usd_amount() - transaction_amount);

    change_clients_balances(sell_order, buy_order_iterator, transaction_amount, transaction_cost);
    std::cout << "clients_balances has changed" << std::endl;

    spdlog::info("Matched orders: BUY {} SELL {} - Amount: {} Cost: {}",
                                     buy_order_iterator->user_jwt(), sell_order.user_jwt(), transaction_amount, transaction_cost);
}

void Core::change_clients_balances(Serialize::TradeOrder& sell_order, std::vector<Serialize::TradeOrder>::iterator buy_order_iterator,
                                   int32_t transaction_amount, double transaction_cost) {
    std::lock_guard<std::mutex> change_client_balance_lock_guard(ptr_session_manager_->client_data_mutex);
    ptr_session_manager_->change_client_balance(sell_order.user_jwt(), DECREASE, USD, transaction_amount);
    ptr_session_manager_->change_client_balance(sell_order.user_jwt(), INCREASE, RUB, transaction_cost);
    ptr_session_manager_->change_client_balance(buy_order_iterator->user_jwt(), INCREASE, USD, transaction_amount);
    ptr_session_manager_->change_client_balance(buy_order_iterator->user_jwt(), DECREASE, RUB, transaction_cost);
}
