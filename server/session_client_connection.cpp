#include "session_client_connection.hpp"

#include "core.hpp"
#include "proto_example.pb.h"

SessionClientConnection::SessionClientConnection(boost::asio::ip::tcp::socket socket, Core& core) 
    : socket_(std::move(socket)), core_(core) {
    std::cout << "New client connected" << std::endl;
}

void SessionClientConnection::start () {
    async_read_data_from_socket();
}

void SessionClientConnection::async_read_data_from_socket() {
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

void SessionClientConnection::async_write_data_to_socket(std::size_t length) {
    auto self_ptr(shared_from_this());
    boost::asio::async_write(socket_, boost::asio::buffer(data_, length), 
        [this, self_ptr](boost::system::error_code ec, std::size_t length){
            if (!ec) {
                async_read_data_from_socket();
            }
        });
}
