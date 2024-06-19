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
                 spdlog::info("Failed to read message length from socket: {}", error_code.message());
                if (error_code == boost::asio::error::eof) {
                    spdlog::info("Client {} has closed socket", get_client_endpoint_info());
                }
                if (error_code == boost::asio::error::connection_reset) {
                    spdlog::info("Connection with client {} was lost", get_client_endpoint_info());
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
                        Serialize::TradeRequest request = convert_raw_data_to_command(length);
                        Serialize::TradeResponse response = handle_received_command(request);

                        async_write_data_to_socket(response);
                    }
                });
            }
        });
}

Serialize::TradeRequest SessionClientConnection::convert_raw_data_to_command(std::size_t length) {
    Serialize::TradeRequest request; 
    request.ParseFromArray(raw_data_from_socket_.data(), length);
    return request;
}

Serialize::TradeResponse SessionClientConnection::handle_received_command(Serialize::TradeRequest request) {
    Serialize::TradeResponse response;
    
    switch (request.command()) {
        case Serialize::TradeRequest::SIGN_UP : {
            //handle_sing_up_command(response, request);
            //response.set_response_msg(Serialize::TradeResponse::SIGN_UP_SUCCESSFUL);
            break;
        }

        case Serialize::TradeRequest::SIGN_IN : {
            handle_sing_in_command(request); // проверочка 
            response.set_response_msg(Serialize::TradeResponse::SIGN_IN_SUCCESSFUL);
            break;
        }

        case Serialize::TradeRequest::MAKE_ORDER : {
            if(!handle_make_order_comand(request)) {
                response.set_response_msg(Serialize::TradeResponse::ERROR); 
                break; 
            }
            response.set_response_msg(Serialize::TradeResponse::ORDER_SUCCESSFULLY_CREATED);
            break;
        }

        case Serialize::TradeRequest::VIEW_BALANCE : {
            if (!handle_view_balance_comand(request, response)) {
                response.set_response_msg(Serialize::TradeResponse::ERROR);
                break;
            } 
            response.set_response_msg(Serialize::TradeResponse::SUCCES_VIEW_BALANCE_RESPONCE);
            break;
        }
            
        case Serialize::TradeRequest::VIEW_ACTIVE_ORDERS : {
            /* code */
            break;
        }
            
        case Serialize::TradeRequest::VIEW_COMPLETED_TRADES : {
            /* code */
            break;
        }
            
        case Serialize::TradeRequest::VIEW_QUOTE_HISTORY : {
            /* code */
            break;
        }

        case Serialize::TradeRequest::CANCEL_ACTIVE_ORDERS : {
            /* code */
            break;
        }

        default: {
            response.set_response_msg(Serialize::TradeResponse::ERROR); 
            break;
        }
    }

    return response;
}

bool SessionClientConnection::handle_sing_in_command(Serialize::TradeRequest request) {
    std::string username = request.username();
    // /? lock_guard
    if (session_manager_->client_data.find(username) == session_manager_->client_data.end()) {
        session_manager_->client_data[username] = ClientData{username, 0.0, 0.0, {}, {}};
    }
    return true;
}

bool SessionClientConnection::handle_make_order_comand(Serialize::TradeRequest request) {
    Serialize::TradeOrder order = request.order();
    order.set_timestamp(get_current_timestamp());

    if (!push_received_from_socket_order_to_queue(order)) {
        spdlog::info("Error to push received from socket order to orders queue : user={} cost={} amount={} type={}",
                     request.username(),
                     order.usd_cost(), order.usd_amount(), 
                     (request.order().type() == Serialize::TradeOrder::BUY) ? "BUY" : "SELL"); // order vs request
        
        return false;
    }
        spdlog::info("New order placed: user={} cost={} amount={} type={}", request.username(),
                 request.order().usd_cost(), request.order().usd_amount(), 
                (request.order().type() == Serialize::TradeOrder::BUY) ? "BUY" : "SELL");

    push_received_from_socket_order_to_active_orders_vector(order, request);

    session_manager_->notify_order_received();

    return true;
}

int64_t SessionClientConnection::get_current_timestamp() {
    auto now = std::chrono::system_clock::now();
    auto now_ms = std::chrono::time_point_cast<std::chrono::milliseconds>(now);
    auto epoch = now_ms.time_since_epoch();
    int64_t timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(epoch).count();
    return timestamp;
}

void SessionClientConnection::push_received_from_socket_order_to_active_orders_vector(Serialize::TradeOrder& order,  Serialize::TradeRequest request) {
    std::lock_guard<std::mutex>  push_back_new_order_lock_guard(session_manager_->client_data_mutex);
    session_manager_->client_data[request.username()].active_orders.push_back(order);
}

bool SessionClientConnection::push_received_from_socket_order_to_queue(const Serialize::TradeOrder& order) {
    switch (order.type()) {
    case Serialize::TradeOrder::BUY :
        return session_manager_->buy_orders_queue->push(order); //TODO setter session_manager_->push_order_to_orders_queue() 
    case Serialize::TradeOrder::SELL :
        return session_manager_->sell_orders_queue->push(order);
    default:
        return false;
    }
}   

bool SessionClientConnection::handle_view_balance_comand(Serialize::TradeRequest request, Serialize::TradeResponse& responce) {
    std::lock_guard<std::mutex> view_balance_lockguard(session_manager_->client_data_mutex);

    Serialize::AccountBalance account_balance = session_manager_->get_client_balance(request.username());
    responce.mutable_account_balance()->CopyFrom(account_balance);

    return true;
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

std::string SessionClientConnection::get_client_username() const {
    return username_;
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
