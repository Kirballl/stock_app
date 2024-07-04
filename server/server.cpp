#include "server.hpp"

Server::Server(boost::asio::io_context& io_context, const Config& config) :
        io_context_(io_context),
        acceptor_(io_context, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), config.port)),
        session_manager_(std::make_shared<SessionManager>()) {
    spdlog::info("\nServer launched! Listen  {} : {}", config.host, config.port);

    session_manager_->init_database();
    session_manager_->init_core();
    session_manager_->init_client_data_manager();
    session_manager_->init_auth();
    try {
        auto database = session_manager_->get_database();
        database->truncate_active_orders_table();
    } catch (const std::exception& e) {
        spdlog::error("Error to truncate active orders table: {}", e.what());
    }

    start();
}

void Server::start() {
    session_manager_thread_ = std::thread(&SessionManager::run, session_manager_);
    core_thread_ = std::thread(&Core::stock_loop, session_manager_->get_core()); 
    
    accept_new_connection();
}

//*INFO Accept new connection thread
void Server::accept_new_connection() {
    acceptor_.async_accept(
        [this](boost::system::error_code ec, boost::asio::ip::tcp::socket new_connectoin_socket) {
            if (!ec) {
                session_manager_->add_new_connection(std::move(new_connectoin_socket));
            } else {
                spdlog::error("Error accepting new connection {}", ec.message());
            }
            accept_new_connection();
        });
}


void Server::stop() {
    spdlog::info("Stopping server...");

    session_manager_->stop();
    spdlog::info("session_manager_ stopped...");

    io_context_.stop();
    spdlog::info("io_context_ stopped...");

    if (session_manager_thread_.joinable()) {
        session_manager_thread_.join();
    }
    spdlog::info("session_manager_thread_ joined");
    if (core_thread_.joinable()) {
        core_thread_.join();
    }
    spdlog::info("core_thread_ joined");
    spdlog::info("Server stopped");
}
