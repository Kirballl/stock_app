#include "server.hpp"

#include "session_client_connection.hpp"

Server::Server(boost::asio::io_context& io_context, const Config& config)
    : io_context_(io_context), acceptor_(io_context, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), config.port)) {
    std::cout << "Server started! Listen " << config.host << ":" << config.port << std::endl;
    accept_new_connection();
}

void Server::accept_new_connection() {
    acceptor_.async_accept(
        [this](boost::system::error_code ec, boost::asio::ip::tcp::socket socket) {
            if (!ec) {
                auto session_ptr = std::make_shared<SessionClientConnection>(std::move(socket), core_);
                session_ptr->start();
            }
            accept_new_connection();
        });
}
