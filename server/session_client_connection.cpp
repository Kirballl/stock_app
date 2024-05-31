#include "session_client_connection.hpp"

Session_client_connection::Session_client_connection(boost::asio::ip::tcp::socket socket) 
    : socket_(std::move(socket)) {
    std::cout << "New client connected" << std::endl;
}

void Session_client_connection::start () {
    async_read_data_from_socket();
}

void Session_client_connection::async_read_data_from_socket() {
    auto self_ptr(shared_from_this());
    socket_.async_read_some(boost::asio::buffer(data_, max_message_length),
        [this, self_ptr](boost::system::error_code ec, std::size_t length) {
            if (!ec) {
                std::cout << "Received data: " << std::string(data_, length) 
                    << " from someone" << std::endl;
                async_write_data_to_socket(length);
            }
        });
}

void Session_client_connection::async_write_data_to_socket(std::size_t length) {
    auto self_ptr(shared_from_this());
    boost::asio::async_write(socket_, boost::asio::buffer(data_, length), 
        [this, self_ptr](boost::system::error_code ec, std::size_t length){
            if (!ec) {
                async_read_data_from_socket();
            }
        });
}
