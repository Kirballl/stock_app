#include "session_client_connection.hpp"

SessionClientConnection::SessionClientConnection(boost::asio::ip::tcp::socket socket, 
        std::shared_ptr<SessionManager> session_manager)
        : socket_(std::move(socket)), session_manager_(session_manager) {
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
    // Read message length
    boost::asio::async_read(socket_,boost::asio::buffer(raw_data_length_from_socket_, sizeof(uint32_t)),
        [this, self_ptr](boost::system::error_code error_code, std::size_t length) {
            if (error_code) {
                spdlog::warn("Failed to read message length from socket: {}", error_code.message());
                if (error_code == boost::asio::error::eof) {
                    spdlog::warn("Client {} has closed socket", get_client_endpoint_info());
                }
                if (error_code == boost::asio::error::connection_reset) {
                    spdlog::warn("Connection with client {} was lost", get_client_endpoint_info());
                }
                close_this_session();
            } else {

            uint32_t msg_length = ntohl(*reinterpret_cast<uint32_t*>(raw_data_length_from_socket_));
            raw_data_from_socket_.resize(msg_length);

            // Read message body
            boost::asio::async_read(socket_, boost::asio::buffer(raw_data_from_socket_, msg_length),
                [this, self_ptr](boost::system::error_code error_code, std::size_t length) {
                    if (error_code) {
                        spdlog::error("Failed to read command from client {} : {}", get_client_endpoint_info(), error_code.message());
                        if (error_code == boost::asio::error::eof) {
                            spdlog::warn("Client {} has closed socket", get_client_endpoint_info());
                        }
                        if (error_code == boost::asio::error::connection_reset) {
                            spdlog::warn("Connection with client {} was lost", get_client_endpoint_info());
                        }
                        close_this_session();
                    } else {
                        async_write_data_to_socket(handle_received_command(convert_raw_data_to_command(length)));
                    }
                });
            }
        });
}

Serialize::TradeRequest SessionClientConnection::convert_raw_data_to_command(std::size_t length) {
    Serialize::TradeRequest trade_request; 
    trade_request.ParseFromArray(raw_data_from_socket_.data(), length);
    return trade_request;
}

Serialize::TradeResponse SessionClientConnection::handle_received_command(Serialize::TradeRequest trade_request) {
    Serialize::TradeResponse response;
    
    switch (trade_request.command()) {
    case Serialize::TradeRequest::MAKE_ORDER :
        if (push_received_from_socket_order_to_queue(trade_request.order())) {
            response.set_response_msg(Serialize::TradeResponse::ORDER_SUCCESSFULLY_CREATED);

            spdlog::info("New order placed: user={} cost={} amount={} type={}", trade_request.username(),
                         trade_request.order().usd_cost(), trade_request.order().usd_amount(), 
                         (trade_request.order().type() == Serialize::TradeOrder::BUY) ? "BUY" : "SELL");
            } else {
                response.set_response_msg(Serialize::TradeResponse::ERROR);
            }
        break;
    case Serialize::TradeRequest::VIEW_ACTIVE_ORDERS :
        /* code */
        break;
    case Serialize::TradeRequest::VIEW_CONPLETED_TRADES :
        /* code */
        break;
    case Serialize::TradeRequest::VIEW_QUOTE_HISTORY :
        /* code */
        break;
    case Serialize::TradeRequest::CANCEL_ACTIVE_ORDERS :
        /* code */
        break;
    default:
        break;
    }

    return response;
}

bool SessionClientConnection::push_received_from_socket_order_to_queue(const Serialize::TradeOrder& order) {
    switch (order.type()) {
    case Serialize::TradeOrder::BUY :
        return session_manager_->buy_orders_queue_->push(order); //? session_manager_->push_order_to_orders_queue() 
    case Serialize::TradeOrder::SELL :
        return session_manager_->sell_orders_queue_->push(order);
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
        [this, self_ptr](boost::system::error_code error_code, std::size_t length) {
            if (error_code) {
                spdlog::error("Failed to send response to client {} : {}", get_client_endpoint_info(), error_code.message());
                if (error_code == boost::asio::error::eof) {
                    spdlog::warn("Client {} has closed socket", get_client_endpoint_info());
                }
                if (error_code == boost::asio::error::connection_reset) {
                    spdlog::warn("Connection with client {} was lost", get_client_endpoint_info());
                }
                close_this_session();
            } 

            async_read_data_from_socket();
        });
}

std::string SessionClientConnection::get_client_endpoint_info() const {
    try {
        return socket_.remote_endpoint().address().to_string() + ":" + std::to_string(socket_.remote_endpoint().port());
    } catch (boost::system::system_error& error) {
        spdlog::error("Failed to get client endpoint info: {}", error.what());
        return "unknown";
    }
}

void SessionClientConnection::close_this_session() {
    std::string client_endpoint_info = get_client_endpoint_info();

    boost::system::error_code error_code;
    socket_.close(error_code);
    if (error_code) {
        spdlog::error("Error closing socket for client {}: {}", client_endpoint_info, error_code.message());
    } else {
        spdlog::info("Connection closed for client {}", client_endpoint_info);
    }

    session_manager_->remove_session(shared_from_this(), client_endpoint_info);
}
