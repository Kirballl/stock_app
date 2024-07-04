#include "client_data_manager.hpp"

ClientDataManager::ClientDataManager(std::shared_ptr<SessionManager> session_manager) : session_manager_(session_manager),
                                         buy_orders_queue_(std::make_shared<OrderQueue>()),
                                         sell_orders_queue_(std::make_shared<OrderQueue>()) {
}

void ClientDataManager::initialize_from_database() {
    auto database = session_manager_->get_database();

    auto clients_balances = database->load_clients_balances_from_db();
    for (const auto& client_balance : clients_balances) {
        clients_funds_data_[client_balance.username()] = client_balance.funds();
    }

    auto active_buy_orders = database->load_active_orders_from_db(Serialize::TradeOrder::BUY);
    auto active_sell_orders = database->load_active_orders_from_db(Serialize::TradeOrder::SELL);
    for (const auto& order : active_buy_orders) {
        active_buy_orders_[order.order_id()] = order;
    }
    for (const auto& order : active_sell_orders) {
        active_sell_orders_[order.order_id()] = order;
    }

    auto last_completed_orders = database->load_last_completed_orders(AMOUNT_LAST_COMPLETED_OREDRS);;
    for (const auto& completed_order : last_completed_orders) {
        completed_orders_.push_back(completed_order);
    }

    auto qoute_history = database->load_quote_history(AMOUNT_QUOTE_HISTORY);
    for (const auto& quote : qoute_history) {
        quote_history_.push_back(quote);
    }
}

//                                                                                //
//                                 Core operations                                //
//                                                                                //
bool ClientDataManager::update_active_order_usd_amount(const Serialize::TradeOrder& order, int32_t transaction_amount) {
    std::unique_lock<std::shared_mutex> update_active_order_usd_amount_unique_lock(client_data_mutex_);

    auto& target_orders = (order.type() == Serialize::TradeOrder::BUY) ? active_buy_orders_ : active_sell_orders_;
    if (target_orders.find(order.order_id()) == target_orders.end()) {
        spdlog::error("Order with id={} in unordered map active orders not found", order.order_id());
        return false;
    }

    target_orders[order.order_id()].set_usd_amount(target_orders[order.order_id()].usd_amount() - transaction_amount);
    return true;
}

bool ClientDataManager::change_client_balances_according_match(const std::string& client_sell, const std::string& client_buy,
                                                              int32_t transaction_amount, double transaction_cost) {
    
    {
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
    }
    
    if (!update_clients_balances_in_db(client_sell, client_buy)) {
        return false;
    }

    return true; 
}

bool ClientDataManager::change_client_balance(const std::string& client_username,
        change_balance_type_t change_balance_type, wallet_type_t wallet_type, double amount) {

    auto client_data_iterator = clients_funds_data_.find(client_username);
    if (client_data_iterator == clients_funds_data_.end()) {
        spdlog::error("client with {} in unordered map client_data not found", client_username);
        return false;
    }

    Serialize::AccountBalance& account_balance = client_data_iterator->second;
    
    if (change_balance_type == INCREASE) {
        wallet_type == RUB ? (account_balance.set_rub_balance(account_balance.rub_balance() + amount)) : 
                             (account_balance.set_usd_balance(account_balance.usd_balance() + amount));
    }
    if (change_balance_type == DECREASE) {
        wallet_type == RUB ? (account_balance.set_rub_balance(account_balance.rub_balance() - amount)) : 
                             (account_balance.set_usd_balance(account_balance.usd_balance() - amount));
    }
    spdlog::info("updated balance in client_data_manager for {} actual USD={}, RUB={}",
                     client_username, account_balance.usd_balance(), account_balance.rub_balance());
    return true;
}

bool ClientDataManager::update_clients_balances_in_db(const std::string& client_sell, const std::string& client_buy) {
    try {
        Serialize::ClientBalance sell_client_balance;
        Serialize::ClientBalance buy_client_balance;
        sell_client_balance.set_username(client_sell);
        sell_client_balance.mutable_funds()->CopyFrom(get_client_balance(client_sell));
        buy_client_balance.set_username(client_buy);
        buy_client_balance.mutable_funds()->CopyFrom(get_client_balance(client_buy));

        auto database = session_manager_->get_database();
        database->update_actual_client_balance_in_db(sell_client_balance);
        database->update_actual_client_balance_in_db(buy_client_balance); 

        return true;
    } catch (const std::exception& e) {
        spdlog::error("Failed to update client balances in database: {}", e.what());
        return false;
    }
}

bool ClientDataManager::add_order_to_completed(const Serialize::TradeOrder& completed_order) {
    std::unique_lock<std::shared_mutex> add_order_to_completed_unque_lock(client_data_mutex_);

    int64_t completion_timestamp = TimeOrderUtils::get_current_timestamp();

    if (!remove_order_from_active_orders(completed_order.order_id(), completed_order.type())) {
        spdlog::error("Failed to remove_order_from_active_orders, order id: {}", completed_order.order_id());
        std::cerr << "Failed to remove_order_from_active_orders, order id: " << completed_order.order_id()  << std::endl;
    }

    Serialize::Quote quote;
    quote.set_price(completed_order.usd_cost());
    quote.set_timestamp(completion_timestamp);
    

    quote_history_.push_back(quote);
    spdlog::info("Quote: price={} added to quote_history_ in client_data_manager", completed_order.usd_cost());
    completed_orders_.push_back(completed_order);
    spdlog::info("Order id:{} added to completed_orders_ in client_data_manager", completed_order.order_id());

    try {
        auto database = session_manager_->get_database();
        database->save_completed_order_to_db(completed_order, completion_timestamp);
        database->save_qoute_to_db(quote);

    } catch (const std::exception& e) {
        spdlog::error("Failed to save completed order or quote to db: {}", e.what());
        std::cerr << "Failed to save completed order or quote to db: " << e.what() << std::endl;
        return false;
    }

    return true;
}

bool ClientDataManager::remove_order_from_active_orders(int64_t order_id, Serialize::TradeOrder::TradeType type){
    auto& target_orders = (type == Serialize::TradeOrder::BUY) ? active_buy_orders_ : active_sell_orders_;

    auto active_order_iterator = target_orders.find(order_id);
    if (active_order_iterator == target_orders.end()) {
        return false;
    }

    target_orders.erase(active_order_iterator);
    return true;
}

//                                                                                //
//                      SessionClientConnection operations                        //
//                                                                                //
void ClientDataManager::create_new_client_fund_data(std::string new_key) {
    std::unique_lock<std::shared_mutex> create_new_client_fund_data_unique_lock(client_data_mutex_);

    Serialize::AccountBalance new_account_balance;
    new_account_balance.set_usd_balance(0.0);
    new_account_balance.set_rub_balance(0.0);

    clients_funds_data_[new_key] = new_account_balance;
    spdlog::info("New client_fund_data username={} in client_data_manager created", new_key);
}

void ClientDataManager::push_order_to_active_orders(const Serialize::TradeOrder& order) {
    std::unique_lock<std::shared_mutex> push_order_to_active_orders_unique_lock(client_data_mutex_);
    if (order.type() == Serialize::TradeOrder::BUY) {
        active_buy_orders_[order.order_id()] = order;
    } else if (order.type() == Serialize::TradeOrder::SELL)  {
        active_sell_orders_[order.order_id()] = order;
    }
}

Serialize::AccountBalance ClientDataManager::get_client_balance(const std::string& client_username) const {
    std::shared_lock<std::shared_mutex> get_client_balance_shared_lock(client_data_mutex_);

    auto hash_map_iterator = clients_funds_data_.find(client_username);
    if (hash_map_iterator == clients_funds_data_.end()) {
        spdlog::error("client with {} in unordered map client_data not found", client_username);
        throw std::runtime_error("client not found");
    }

    Serialize::AccountBalance account_balance = hash_map_iterator->second;

    return account_balance;
}

Serialize::ActiveOrders ClientDataManager::get_all_active_oreders() {
    Serialize::ActiveOrders all_active_orders;
    std::shared_lock<std::shared_mutex> get_all_active_oreders_shared_lock(client_data_mutex_);
    
    for (const auto& [order_id, order] : active_buy_orders_) {
        *all_active_orders.add_active_buy_orders() = order;
    }
    for (const auto& [order_id, order] : active_sell_orders_) {
        *all_active_orders.add_active_sell_orders() = order;
    }

    return all_active_orders;
}

Serialize::CompletedOredrs ClientDataManager::get_last_completed_oreders() {
    Serialize::CompletedOredrs all_completed_orders;
    std::shared_lock<std::shared_mutex> get_all_completed_oreders_shared_lock(client_data_mutex_);

    for (const auto& order : completed_orders_) {
        if (order.type() == Serialize::TradeOrder::BUY) {
            *all_completed_orders.add_completed_buy_orders() = order;
        } else if (order.type() == Serialize::TradeOrder::SELL) {
            *all_completed_orders.add_completed_sell_orders() = order;
        }
    }

    return all_completed_orders;
}

Serialize::QuoteHistory ClientDataManager::get_quote_history() {
    Serialize::QuoteHistory quote_history;
    std::shared_lock<std::shared_mutex> get_quote_history_shared_lock(client_data_mutex_);
    
    for (const auto& quote : quote_history_) {
        *quote_history.add_quotes() = quote;
    }
    
    return quote_history;
}

//                                                                                //
//                     Condition variables notifications                          //
//                                                                                //
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

//                                                                                //
//                             Order queue operations                             //
//                                                                                //
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
