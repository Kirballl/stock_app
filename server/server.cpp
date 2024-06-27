#include "server.hpp"

Server::Server(boost::asio::io_context& io_context, const Config& config) :
        io_context_(io_context),
        acceptor_(io_context, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), config.port)),
        session_manager_(std::make_shared<SessionManager>()) {
    spdlog::info("Server launched! Listen  {} : {}", config.host, config.port);

    session_manager_->init_core();

    std::cout << "Server launched! Listen " << config.host << ":" << config.port << std::endl;
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
                std::cerr << "Error accepting new connection: " << ec.message() << std::endl;
            }
            accept_new_connection();
        });
}


void Server::stop() {
    spdlog::info("Stopping server...");

    std::cout << "Stopping server..." << std::endl;

    session_manager_->stop();

    std::cout << "session_manager_ stopped..." << std::endl;

    io_context_.stop();

    std::cout << "io_context_ stopped..." << std::endl;

    if (session_manager_thread_.joinable()) {
        session_manager_thread_.join();
    }
    std::cout << "session_manager_thread_ joined" << std::endl;
    if (core_thread_.joinable()) {
        core_thread_.join();
    }
    std::cout << "core_thread_ joined" << std::endl;

    spdlog::info("Server stopped");
}
