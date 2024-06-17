#include "session_manager.hpp"

SessionManager::SessionManager() : is_running_(true), handle_sessions_mutex_(),
    buy_orders_queue(std::make_shared<OrderQueue>()), sell_orders_queue(std::make_shared<OrderQueue>()) {
}

bool SessionManager::is_runnig() {
    return is_running_.load(std::memory_order_acquire);
}

// Here session_manager_thread_ 
void SessionManager::run() {
    while (is_running_) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        // Handle active sessions
        try_to_create_new_session_client_connection();
    }
}

void SessionManager::try_to_create_new_session_client_connection() {
    std::unique_ptr<boost::asio::ip::tcp::socket> new_socket;
    while (new_connections_queue_.try_dequeue(new_socket)) {
        auto new_session_client_connection = std::make_shared<SessionClientConnection>(
            std::move(*new_socket), shared_from_this());

        new_session_client_connection->start();

        std::lock_guard add_new_session_lock_guard(handle_sessions_mutex_);
        clients_sessions_.push_back(new_session_client_connection);

        std::cout << "New client connected:" << new_session_client_connection->get_client_endpoint_info() << std::endl;
        spdlog::info("Session added for client {}", new_session_client_connection->get_client_endpoint_info());
    }
}

// Avalible only in main server thread
void SessionManager::add_new_connection(boost::asio::ip::tcp::socket new_client_socket) {
    auto new_socket = std::make_unique<boost::asio::ip::tcp::socket>(std::move(new_client_socket));
    if (!new_connections_queue_.enqueue(std::move(new_socket))) {
        std::cerr << "Failed to add new_socket in new_connections_queue_" << std::endl;
        spdlog::error("Failed to add new_socket in new_connections_queue_");
    }
}

std::shared_ptr<SessionClientConnection> SessionManager::get_session_by_username(const std::string& username) {
    std::lock_guard<std::mutex> get_session_by_username_lock_guard(handle_sessions_mutex_);
    for (auto& current_session : clients_sessions_) {
        if (current_session->get_client_username() == username) {
            return current_session;
        }
    }

    spdlog::warn("Try to get session by unknown username {}");
    return nullptr;
}

double SessionManager::get_client_balance(std::string client_jwt, wallet_type_t wallet_type) const {
    // std::lock_guard<std::mutex> get_client_balance_lock_guard(client_data_mutex);
    auto hash_map_iterator = client_data.find(client_jwt);
    if (hash_map_iterator == client_data.end()) {
        spdlog::error("client with {} in unordered map client_data not found", client_jwt);
        throw std::runtime_error("client not found");
    }
    return wallet_type == RUB ? hash_map_iterator->second.rub_balance : hash_map_iterator->second.usd_balance;
}

Serialize::AccountBalance SessionManager::get_client_balance(std::string client_jwt) const {
    // std::lock_guard<std::mutex> get_client_balance_lock_guard(client_data_mutex);

    auto hash_map_iterator = client_data.find(client_jwt);
    if (hash_map_iterator == client_data.end()) {
        spdlog::error("client with {} in unordered map client_data not found", client_jwt);
        throw std::runtime_error("client not found");
    }
    Serialize::AccountBalance account_balance;
    account_balance.set_rub_balance(hash_map_iterator->second.rub_balance);
    account_balance.set_usd_balance(hash_map_iterator->second.usd_balance);

    return account_balance;
}

void SessionManager::change_client_balance(std::string client_jwt,
        change_balance_type_t change_balance_type, wallet_type_t wallet_type, double amount) {

    auto iterator = client_data.find(client_jwt);
    if (iterator == client_data.end()) {
        spdlog::error("client with {} in unordered map client_data not found", client_jwt);
        throw std::runtime_error("client not found");
    }

    ClientData& client_funds = iterator->second;
    if (change_balance_type == INCREASE) {
        wallet_type == RUB ? (client_funds.rub_balance += amount) : (client_funds.usd_balance += amount);
    }
    if (change_balance_type == DECREASE) {
        wallet_type == RUB ? (client_funds.rub_balance -= amount) : (client_funds.usd_balance -= amount);
    }
}

void SessionManager::move_order_from_active_to_completed(const std::string& client_jwt, int64_t timestamp) {
    auto client_it = client_data.find(client_jwt);
    if (client_it == client_data.end()) {
        spdlog::error("client with {} in unordered map client_data not found", client_jwt);
        throw std::runtime_error("client not found");
    }

    auto order_it = std::find_if(client_it->second.active_orders.begin(), client_it->second.active_orders.end(),
        [timestamp](const Serialize::TradeOrder& order) {
            return order.timestamp() == timestamp;
        });

    if (order_it == client_it->second.active_orders.end()) {
        spdlog::error("client {} order with {} timestamp in unordered map client_data not found", client_jwt, timestamp);
        throw std::runtime_error("client not found");
    }
    client_it->second.completed_orders.push_back(*order_it);
    client_it->second.active_orders.erase(order_it);
}

void SessionManager::notify_order_received() {
    order_queue_cv.notify_all();
}

void SessionManager::remove_session(std::shared_ptr<SessionClientConnection> session, std::string client_endpoint_info) {
    std::lock_guard<std::mutex> remove_session_lock_guard(handle_sessions_mutex_);
    clients_sessions_.erase(std::remove(clients_sessions_.begin(), clients_sessions_.end(), session), clients_sessions_.end());
    spdlog::info("Session removed for client {}", client_endpoint_info);
}

void SessionManager::stop() {
    spdlog::info("Stopping server session namager...");

    is_running_.store(false, std::memory_order_release);

    std::lock_guard<std::mutex> stop_session_manager_lock_guard(handle_sessions_mutex_);
    for (auto& current_session : clients_sessions_) {
        current_session->close_this_session();
    }

    order_queue_cv.notify_all();
}
