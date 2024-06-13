#include "server.hpp"

Server::Server(boost::asio::io_context& io_context, const Config& config) :
        io_context_(io_context),
        acceptor_(io_context, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), config.port)),
        session_manager_(std::make_shared<SessionManager>()) {
    spdlog::info("Server launched! Listen  {} : {}", config.host, config.port);

    std::cout << "Server launched! Listen " << config.host << ":" << config.port << std::endl;
    start();
}

void Server::start() {
    session_manager_thread_ = std::thread(&SessionManager::run, session_manager_);
    matching_orders_thread_ = std::thread(&Server::stock_core, this);
    
    accept_new_connection();
}

// Accept new connection thread
void Server::accept_new_connection() {
    acceptor_.async_accept(
        [this](boost::system::error_code ec, boost::asio::ip::tcp::socket new_connectoin_socket) {
            if (!ec) {
                session_manager_->add_new_connection(std::move(new_connectoin_socket));
            } else {
                spdlog::error("Error accepting new connection {}", ec.message());
                std::cerr << "Error accepting new connection: " << ec.message() << std::endl;
            }
            accept_new_connection();
        });
}

// matching orders thread
void Server::stock_core() {
    while (session_manager_->is_runnig()) {
        std::unique_lock<std::mutex> order_queue_unique_lock(session_manager_->order_queue_cv_mutex);

        // Wait order from order queue 
        session_manager_->order_queue_cv.wait(order_queue_unique_lock, [this] {
            return !session_manager_->buy_orders_queue->is_empty()  ||
                   !session_manager_->sell_orders_queue->is_empty() ||
                   !session_manager_->is_runnig();
        });

        
        if (!session_manager_->is_runnig()) {
            break;
        }


        match_orders();
    }
}

void Server::match_orders() {
    while (!session_manager_->buy_orders_queue->is_empty() &&
           !session_manager_->sell_orders_queue->is_empty()) {
    }

    //spdlog::info("Trying to match orders");
}

void Server::stop() {
    spdlog::info("Stopping server...");

    session_manager_->stop();

    io_context_.stop();

    if (session_manager_thread_.joinable()) {
        session_manager_thread_.join();
    }
    if (matching_orders_thread_.joinable()) {
        matching_orders_thread_.join();
    }

    spdlog::info("Server stopped");
}
