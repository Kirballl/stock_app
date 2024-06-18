#include "server.hpp"

Server::Server(boost::asio::io_context& io_context, const Config& config) :
        io_context_(io_context),
        acceptor_(io_context, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), config.port)),
        session_manager_(std::make_shared<SessionManager>()), 
        core_(std::make_shared<Core>(session_manager_)) {
    spdlog::info("Server launched! Listen  {} : {}", config.host, config.port);

    std::cout << "Server launched! Listen " << config.host << ":" << config.port << std::endl;
    start();
}

void Server::start() {
    session_manager_thread_ = std::thread(&SessionManager::run, session_manager_);
    core_thread_ = std::thread(&Core::stock_loop, core_);
    
    accept_new_connection();
}

//INFO Accept new connection thread
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


void Server::stop() {
    spdlog::info("Stopping server...");

    session_manager_->stop();

    io_context_.stop();

    if (session_manager_thread_.joinable()) {
        session_manager_thread_.join();
    }
    if (core_thread_.joinable()) {
        core_thread_.join();
    }

    spdlog::info("Server stopped");
}
