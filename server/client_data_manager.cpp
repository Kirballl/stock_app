#include "client_data_manager.hpp"

ClientDataManager::ClientDataManager() : buy_orders_queue_(std::make_shared<OrderQueue>()),
                                         sell_orders_queue_(std::make_shared<OrderQueue>()) {
}

Serialize::AccountBalance ClientDataManager::get_client_balance(const std::string& client_username) const {

    std::shared_lock<std::shared_mutex> get_client_balance_shared_lock(client_data_mutex_);

    auto hash_map_iterator = client_data_.find(client_username);
    if (hash_map_iterator == client_data_.end()) {
        spdlog::error("client with {} in unordered map client_data not found", client_username);
        throw std::runtime_error("client not found");
    }
    Serialize::AccountBalance account_balance;
    account_balance.set_rub_balance(hash_map_iterator->second.rub_balance);
    account_balance.set_usd_balance(hash_map_iterator->second.usd_balance);

    return account_balance;
}

bool ClientDataManager::change_client_balances_according_match(const std::string& client_sell, const std::string& client_buy,
                                                              int32_t transaction_amount, double transaction_cost) {
    std::unique_lock<std::shared_mutex> change_client_balance_unique_lock(client_data_mutex_);    

    if (!change_client_balance(client_sell, DECREASE, USD, transaction_amount)) {
        return false;
    }
    if (!change_client_balance(client_sell, INCREASE, RUB, transaction_cost)) {
        return false;
    }
    if (!change_client_balance(client_buy, INCREASE, USD, transaction_amount)) {
        return false;
    }
    if (!change_client_balance(client_buy, DECREASE, RUB, transaction_cost)) {
        return false;
    }

    return true; 
}

bool ClientDataManager::change_client_balance(const std::string& client_username,
        change_balance_type_t change_balance_type, wallet_type_t wallet_type, double amount) {

    auto client_data_iterator = client_data_.find(client_username);
    if (client_data_iterator == client_data_.end()) {
        spdlog::error("client with {} in unordered map client_data not found", client_username);
        return false;
    }

    if (change_balance_type == INCREASE) {
        wallet_type == RUB ? (client_data_iterator->second.rub_balance += amount) : 
                             (client_data_iterator->second.usd_balance += amount);
    }
    if (change_balance_type == DECREASE) {
        wallet_type == RUB ? (client_data_iterator->second.rub_balance -= amount) : 
                             (client_data_iterator->second.usd_balance -= amount);
    }
    return true;
}

bool ClientDataManager::push_order_to_active_orders(const std::string& client_username, Serialize::TradeOrder& order) {

    std::unique_lock<std::shared_mutex>  push_order_to_active_orders_unque_lock(client_data_mutex_);

    auto client_data_iterator = client_data_.find(client_username);
    if (client_data_iterator == client_data_.end()) {
        spdlog::error("client with {} in unordered map client_data not found", client_username);
        return false;
    }

    client_data_iterator->second.active_orders.push_back(order);
    return true;
}

bool ClientDataManager::change_order_usd_amount_according_match(const std::string& client_username, int64_t timestamp, int32_t transaction_amount) {
    std::unique_lock<std::shared_mutex> change_order_usd_amount_according_match_unque_lock(client_data_mutex_);

    auto [order_iterator, is_found] = find_order_by_username_and_timestamp(client_username, timestamp);
    if (!is_found) {
        return false;
    }
    
    order_iterator->set_usd_amount(order_iterator->usd_amount() - transaction_amount);

    spdlog::info("Order was partially executed : user={} order_id={}  cost={} amount={} type={}",
                 order_iterator->username(), order_iterator->order_id(), order_iterator->usd_cost(), order_iterator->usd_amount(), 
                (order_iterator->type() == Serialize::TradeOrder::BUY) ? "BUY" : "SELL");
    return true;
}

bool ClientDataManager::move_order_from_active_to_completed(const std::string& client_username, int64_t timestamp) {

    std::unique_lock<std::shared_mutex> move_order_from_active_to_completed_unque_lock(client_data_mutex_);

    auto [order_iterator, found] = find_order_by_username_and_timestamp(client_username, timestamp);
    if (!found) {
        return false;
    }
    auto client_data_iterator = client_data_.find(client_username);

    spdlog::info("Order was executed: user={} order_id={} cost={} amount={} type={}",
                 order_iterator->username(), order_iterator->order_id(), order_iterator->usd_cost(), order_iterator->usd_amount(), 
                (order_iterator->type() == Serialize::TradeOrder::BUY) ? "BUY" : "SELL");

    client_data_iterator->second.completed_orders.push_back(*order_iterator);
    client_data_iterator->second.active_orders.erase(order_iterator);
    return true;
}

std::pair<std::vector<Serialize::TradeOrder>::iterator, bool> ClientDataManager::find_order_by_username_and_timestamp(
    const std::string& client_username, int64_t timestamp) {

    auto client_data_iterator = client_data_.find(client_username);
    if (client_data_iterator == client_data_.end()) {
        spdlog::error("Client with username {} not found", client_username);
        return {std::vector<Serialize::TradeOrder>::iterator{}, false};
    }

    auto& active_orders = client_data_iterator->second.active_orders;
    auto order_iterator = std::find_if(active_orders.begin(), active_orders.end(),
        [timestamp](const Serialize::TradeOrder& order) {
            return order.timestamp() == timestamp;
        });

    if (order_iterator == active_orders.end()) {
        spdlog::error("Order with timestamp {} for client {} not found", timestamp, client_username);
        return {std::vector<Serialize::TradeOrder>::iterator{}, false};
    }

    return {order_iterator, true};
}

bool ClientDataManager::has_username_in_client_data(std::string key) {
    std::shared_lock<std::shared_mutex> has_username_in_client_data_shared_lock(client_data_mutex_);
    return client_data_.find(key) != client_data_.end();
}

void ClientDataManager::create_new_client_data(std::string new_key) {
    std::unique_lock<std::shared_mutex> create_new_client_data_unique_lock(client_data_mutex_);
    client_data_[new_key] = ClientData{new_key, 0.0, 0.0, {}, {}};
}

Serialize::ActiveOrders ClientDataManager::get_all_active_oreders() {
    Serialize::ActiveOrders all_active_orders;
    std::shared_lock<std::shared_mutex> get_all_active_oreders_shared_lock(client_data_mutex_);

    for (const auto& client_pair : client_data_) {
        const auto& client_active_orders = client_pair.second.active_orders;
        for (const auto& active_order : client_active_orders) {
            if (active_order.type() == Serialize::TradeOrder::BUY) {
                *all_active_orders.add_active_buy_orders() = active_order;
            } else if (active_order.type() == Serialize::TradeOrder::SELL) {
                *all_active_orders.add_active_sell_orders() = active_order;
            }
        }
    }

    return all_active_orders;
}

Serialize::CompletedOredrs ClientDataManager::get_all_completed_oreders() {
    Serialize::CompletedOredrs all_completed_oreders;
    std::shared_lock<std::shared_mutex> get_all_completed_oreders_shared_lock(client_data_mutex_);

    for (const auto& client_pair : client_data_) {
        const auto& client_completed_orders = client_pair.second.completed_orders;
        for (const auto& completed_order : client_completed_orders) {
            if (completed_order.type() == Serialize::TradeOrder::BUY) {
                *all_completed_oreders.add_completed_buy_orders() = completed_order;
            } else if (completed_order.type() == Serialize::TradeOrder::SELL) {
                *all_completed_oreders.add_completed_sell_orders() = completed_order;
            }
        }
    }

    return all_completed_oreders;
}

bool ClientDataManager::is_empty_order_queue(trade_type_t trade_type) {
    if (trade_type == BUY) {
        return buy_orders_queue_->is_empty();
    } else {
        return sell_orders_queue_->is_empty();
    }
}

bool ClientDataManager::pop_order_from_order_queue(trade_type_t trade_type, Serialize::TradeOrder& order) {
    if (trade_type == BUY) {
        return buy_orders_queue_->pop(order);
    } else {
        return sell_orders_queue_->pop(order);
    }
}

bool ClientDataManager::push_order_to_order_queue(trade_type_t trade_type, const Serialize::TradeOrder& order) {
    if (trade_type == BUY) {
        return buy_orders_queue_->push(order);
    } else {
        return sell_orders_queue_->push(order);
    }
}

void ClientDataManager::notify_order_received() {
    order_queue_cv_.notify_all();
}

void ClientDataManager::notify_to_stop_matching_orders() {
    order_queue_cv_.notify_all();
}

void ClientDataManager::stock_loop_wait_for_orders(std::shared_ptr<SessionManager> session_manager_ptr) {
    std::unique_lock<std::mutex> order_queue_unique_lock(order_queue_cv_mutex_);

    //*INFO: Wait order from order queue 
    order_queue_cv_.wait(order_queue_unique_lock, [this, session_manager_ptr] {
        return !is_empty_order_queue(BUY)  || !is_empty_order_queue(SELL) ||
                !session_manager_ptr->is_runnig();
    });
}
