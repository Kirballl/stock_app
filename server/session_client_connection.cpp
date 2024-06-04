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

    std::cout << "async_read_data_from_socket()" << std::endl;

    socket_.async_read_some(boost::asio::buffer(data_, max_message_length),
        [this, self_ptr](boost::system::error_code ec, std::size_t length) {
            if (!ec) {
                Serialize::TradeOrder order;
                // Desereialeze data
                order.ParseFromArray(data_, length);

                Serialize::TradeResponse responce = core_.handle_order(order);
                async_write_data_to_socket(responce);
            }
        });
}

void SessionClientConnection::async_write_data_to_socket(const Serialize::TradeResponse& responce) {
    auto self_ptr(shared_from_this());

    std::cout << "async_write_data_to_socket()" << std::endl;

    std::string serialized_response;
    responce.SerializeToString(&serialized_response);
    serialized_response += "\n"; 

    boost::asio::async_write(socket_, boost::asio::buffer(serialized_response), 
        [this, self_ptr](boost::system::error_code ec, std::size_t length) {
            if (!ec) {
                async_read_data_from_socket();
            } else {
                std::cerr << "Failed to send response: " << ec.message() << std::endl;
            }
        });
}
