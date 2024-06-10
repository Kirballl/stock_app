#include "session_manager.hpp"

SessionManager::SessionManager() : is_rinning_(true),
    buy_orders_queue_(std::make_shared<OrderQueue>()), sell_orders_queue_(std::make_shared<OrderQueue>()) {
}

// Here session_manager_thread_ 
void SessionManager::run() {
    while (is_rinning_) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        // Handle active sessions
        start_async_read_wrtite_on_new_connections();
    }
}

void SessionManager::start_async_read_wrtite_on_new_connections() {
    std::unique_ptr<boost::asio::ip::tcp::socket> new_socket;
    while (new_connections_queue_.try_dequeue(new_socket)) {
        auto new_session_client_connection = std::make_shared<SessionClientConnection>(
            std::move(*new_socket), buy_orders_queue_, sell_orders_queue_);
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
    }
}

void SessionManager::stop() {
    std::lock_guard<std::mutex> stop_session_manager_lock_guard(handle_sessions_mutex_);
    is_rinning_ = false;
    for (auto& current_session : clients_sessions_) {
        current_session->close();
        spdlog::info("Closed session for cliet {}", current_session->get_client_endpoint_info());
    }
}
