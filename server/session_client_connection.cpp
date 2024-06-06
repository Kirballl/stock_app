#include "session_client_connection.hpp"

#include "core.hpp"
#include "trade_market_protocol.pb.h"

SessionClientConnection::SessionClientConnection(boost::asio::ip::tcp::socket socket, Core& core) 
    : socket_(std::move(socket)), core_(core) {
    std::cout << "New client connected" << std::endl;
}

void SessionClientConnection::start () {
    async_read_data_from_socket();
}

void SessionClientConnection::async_read_data_from_socket() {
    auto self_ptr(shared_from_this());

    // Read message length
    boost::asio::async_read(socket_,boost::asio::buffer(data_length_, sizeof(uint32_t)),
        [this, self_ptr](boost::system::error_code ec, std::size_t length) {
            if (!ec) {
                uint32_t msg_length = ntohl(*reinterpret_cast<uint32_t*>(data_length_));
                data_.resize(msg_length);

                boost::asio::async_read(socket_, boost::asio::buffer(data_, msg_length),
                    [this, self_ptr](boost::system::error_code ec, std::size_t length) {
                        if (!ec) {
                            Serialize::TradeOrder order;
                            order.ParseFromArray(data_.data(), length); // .data() ?

                            Serialize::TradeResponse responce = core_.handle_order(order);

                            async_write_data_to_socket(responce);
                        } else {
                            std::cerr << "Failed to read order message from socket: " << ec.message() << std::endl;
                        }
                    });
            } else {
                std::cerr << "Failed to read message length from socket: " << ec.message() << std::endl;
            }
        });
}

void SessionClientConnection::async_write_data_to_socket(const Serialize::TradeResponse& responce) {
    auto self_ptr(shared_from_this());

    std::string serialized_response;
    responce.SerializeToString(&serialized_response);

    int32_t message_length = static_cast<int32_t>(serialized_response.length());
    std::string message = std::to_string(message_length) + serialized_response;

    uint32_t msg_length = htonl(static_cast<uint32_t>(serialized_response.size()));
    std::vector<boost::asio::const_buffer> buffers = {
        boost::asio::buffer(&msg_length, sizeof(uint32_t)),
        boost::asio::buffer(serialized_response)
    };

    boost::asio::async_write(socket_, buffers, 
        [this, self_ptr](boost::system::error_code ec, std::size_t length) {
            if (!ec) {
                async_read_data_from_socket();
            } else {
                std::cerr << "Failed to send response: " << ec.message() << std::endl;
            }
        });
}
