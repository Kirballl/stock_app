#include "session_client_connection.hpp"

SessionClientConnection::SessionClientConnection(boost::asio::ip::tcp::socket socket, 
        std::shared_ptr<OrderQueue> buy_orders_queue, std::shared_ptr<OrderQueue> sell_orders_queue)
        : socket_(std::move(socket)), buy_orders_queue_(buy_orders_queue), sell_orders_queue_(sell_orders_queue) {
}

void SessionClientConnection::start () {
    // Auntification
    // read data from socket
    // chaeck in DB
    // if (!) {}
    async_read_data_from_socket();
}

void SessionClientConnection::async_read_data_from_socket() {
    auto self_ptr(shared_from_this());

    //std::cout << "SessionClientConnection::async_read_data_from_socket()" << std::endl;

    // Read message length
    boost::asio::async_read(socket_,boost::asio::buffer(raw_data_length_from_socket_, sizeof(uint32_t)),
        [this, self_ptr](boost::system::error_code ec, std::size_t length) {
            if (!ec) {
                std::cout << "Read message length" << std::endl;

                uint32_t msg_length = ntohl(*reinterpret_cast<uint32_t*>(raw_data_length_from_socket_));
                raw_data_from_socket_.resize(msg_length);

                // Read message body
                boost::asio::async_read(socket_, boost::asio::buffer(raw_data_from_socket_, msg_length),
                    [this, self_ptr](boost::system::error_code ec, std::size_t length) {
                        if (!ec) {
                            std::cout << "Read message body" << std::endl;
                            Serialize::TradeOrder order;
                            order.ParseFromArray(raw_data_from_socket_.data(), length);

                            spdlog::info("New order received");

                            Serialize::TradeResponse response;
                            if (push_received_from_socket_order_to_queue(order)) {
                                response.set_response_msg(Serialize::TradeResponse::ORDER_SUCCESSFULLY_PLACED);

                                spdlog::info("New order placed: user={} cost={} amount={} type={}", 
                                    order.username(), order.usd_cost(), order.usd_amount(), 
                                    (order.type() == Serialize::TradeOrder::BUY) ? "BUY" : "SELL");
                            } else {
                                response.set_response_msg(Serialize::TradeResponse::ERROR);
                            }
                            async_write_data_to_socket(response);
                        } else {
                            std::cerr << "Failed to read order message from socket: " << ec.message() << std::endl;
                        }
                    });
            } else {
                std::cerr << "Failed to read message length from socket: " << ec.message() << std::endl;
            }
        });
}

bool SessionClientConnection::push_received_from_socket_order_to_queue(const Serialize::TradeOrder& order) {
     switch (order.type()) {
    case Serialize::TradeOrder::BUY :
        return buy_orders_queue_->push(order);
    case Serialize::TradeOrder::SELL :
        return sell_orders_queue_->push(order);
    default:
        return false;
    }
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

std::string SessionClientConnection::get_client_endpoint_info() const {
    return socket_.remote_endpoint().address().to_string() + ":" + std::to_string(socket_.remote_endpoint().port());
}

void SessionClientConnection::close() {
    boost::system::error_code ec;
    socket_.close(ec);
    if (ec) {
        spdlog::error("Error closing socket: {}", ec.message());
    }
}
