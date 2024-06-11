#include "session_manager.hpp"

SessionManager::SessionManager() : is_rinning_(true), handle_sessions_mutex_(),
    buy_orders_queue_(std::make_shared<OrderQueue>()), sell_orders_queue_(std::make_shared<OrderQueue>()) {
}

// Here session_manager_thread_ 
void SessionManager::run() {
    while (is_rinning_) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        // Handle active sessions
        create_session_client_connection();
    }
}

void SessionManager::create_session_client_connection() {
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
void SessionManager::add_new_session(boost::asio::ip::tcp::socket new_client_socket) {
    auto new_socket = std::make_unique<boost::asio::ip::tcp::socket>(std::move(new_client_socket));
    if (!new_connections_queue_.enqueue(std::move(new_socket))) {
        std::cerr << "Failed to add new_socket in new_connections_queue_" << std::endl;
        spdlog::error("Failed to add new_socket in new_connections_queue_");
    }
}

void SessionManager::remove_session(std::shared_ptr<SessionClientConnection> session, std::string client_endpoint_info) {
    std::lock_guard<std::mutex> remove_session_lock_guard(handle_sessions_mutex_);
    clients_sessions_.erase(std::remove(clients_sessions_.begin(), clients_sessions_.end(), session), clients_sessions_.end());
    spdlog::info("Session removed for client {}", client_endpoint_info);
}

void SessionManager::stop() {
    spdlog::info("Stopping server session namager...");

    std::lock_guard<std::mutex> stop_session_manager_lock_guard(handle_sessions_mutex_);
    is_rinning_ = false;
    for (auto& current_session : clients_sessions_) {
        current_session->close_this_session();
    }
}
