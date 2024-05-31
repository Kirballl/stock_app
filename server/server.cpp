#include "server.hpp"
#include "session_client_connection.hpp"

Server::Server(boost::asio::io_context& io_context)
    : io_context_(io_context), acceptor_(io_context, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port)) {
    std::cout << "Server started! Listen " << port << " port" << std::endl;
    do_accept();
}

void Server::do_accept() {
    acceptor_.async_accept(
        [this](boost::system::error_code ec, boost::asio::ip::tcp::socket socket) {
            if (!ec) {
                auto session_ptr = std::make_shared<Session_client_connection>(std::move(socket));
                session_ptr->start();
            }
            do_accept();
        });
}
