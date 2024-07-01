#include "session_client_connection.hpp"

SessionClientConnection::SessionClientConnection(boost::asio::ip::tcp::socket socket, 
        std::shared_ptr<SessionManager> session_manager)
        : socket_(std::move(socket)), session_manager_(session_manager) {
}

void SessionClientConnection::start () {
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

Serialize::TradeResponse SessionClientConnection::handle_received_command(Serialize::TradeRequest& request) {
    Serialize::TradeResponse response;

    if (request.command() != Serialize::TradeRequest::SIGN_IN &&
        request.command() != Serialize::TradeRequest::SIGN_UP) {
        auto auth = session_manager_->get_auth();
        //*INFO check jwt
        if(!auth->verify_token(request.jwt(), username_)) {
            std::cerr << "Username sent invalid jwt, request from:" << get_client_endpoint_info() << std::endl;
            spdlog::info("Username {} sent invalid jwt, request from: {}",
                            username_, get_client_endpoint_info());
            response.set_response_msg(Serialize::TradeResponse::ERROR); 

            async_write_data_to_socket(response);
            close_this_session();
        }
    }
    
    switch (request.command()) {
        case Serialize::TradeRequest::SIGN_UP : {
            if (!handle_sing_up_command(response, request)) {
                response.set_response_msg(Serialize::TradeResponse::USERNAME_ALREADY_TAKEN);

                spdlog::info("Username {} already exist in DB, request from: {}",
                            request.sign_up_request().username(), get_client_endpoint_info());
                break;
            }
            response.set_response_msg(Serialize::TradeResponse::SIGN_UP_SUCCESSFUL);

            spdlog::info("Successfull sing up for username: {}, request from: {}", 
                            request.sign_up_request().username(), get_client_endpoint_info());
            break;
        }

        case Serialize::TradeRequest::SIGN_IN : {
            if (!handle_sing_in_command(response, request)) {
                spdlog::info("Invalid sign-in attempt for username: {}, request from: {}",
                     request.sign_in_request().username(), get_client_endpoint_info());
                break;
            }
            response.set_response_msg(Serialize::TradeResponse::SIGN_IN_SUCCESSFUL);

            spdlog::info("Successfull sing-in for username: {}, request from {}", 
                            request.sign_in_request().username(), get_client_endpoint_info());
            break;
        }

        case Serialize::TradeRequest::MAKE_ORDER : {
            if(!handle_make_order_comand(request)) {
                response.set_response_msg(Serialize::TradeResponse::ERROR); 
                break; 
            }
            response.set_response_msg(Serialize::TradeResponse::ORDER_SUCCESSFULLY_CREATED);

            spdlog::info("New order placed: user={} cost={} amount={} type={}", request.username(),
                 request.order().usd_cost(), request.order().usd_amount(), 
                (request.order().type() == Serialize::TradeOrder::BUY) ? "BUY" : "SELL");
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
            
        case Serialize::TradeRequest::VIEW_ALL_ACTIVE_BUY_ORDERS : {
            handle_view_all_active_oreders_command(request, response);
            response.set_response_msg(Serialize::TradeResponse::SUCCES_VIEW_ALL_ACTIVE_ORDERS);
            break;
        }
        case Serialize::TradeRequest::VIEW_ALL_ACTIVE_SELL_ORDERS : {
            handle_view_all_active_oreders_command(request, response);
            response.set_response_msg(Serialize::TradeResponse::SUCCES_VIEW_ALL_ACTIVE_ORDERS);
            break;
        }

        case Serialize::TradeRequest::VIEW_MY_ACTIVE_ORDERS : {
            
            
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

bool SessionClientConnection::handle_sing_up_command(Serialize::TradeResponse& response, Serialize::TradeRequest& request) {
    auto database = session_manager_->get_database();

    if (database->is_user_exists(request.sign_up_request().username())) {
        spdlog::info("User {} is already exist in DB", request.sign_up_request().username());
        return false;
    }

    database->add_user(request.sign_up_request().username(), request.sign_up_request().password());

    auto client_data_manager = session_manager_->get_client_data_manager();
    client_data_manager->create_new_client_fund_data(request.sign_up_request().username());

    return true;
}

bool SessionClientConnection::handle_sing_in_command(Serialize::TradeResponse& response, Serialize::TradeRequest& request) {
    auto database = session_manager_->get_database();

    if (!database->authenticate_user(request.sign_in_request().username(),  request.sign_in_request().password())) {
        response.set_response_msg(Serialize::TradeResponse::INVALID_USERNAME_OR_PASSWORD);
        return false;
    }

    if (session_manager_->is_user_logged_in(request.sign_in_request().username())) {
        spdlog::info("User {} is already logged in, request from: {}",
                     request.sign_in_request().username(), get_client_endpoint_info());
        response.set_response_msg(Serialize::TradeResponse::USER_ALREADY_LOGGED_IN);
        return false;
    }

    username_ = request.sign_in_request().username();
    //*INFO generating jwt
    auto auth = session_manager_->get_auth();
    std::string jwt_token = auth->generate_token(request.sign_in_request().username());
    response.set_jwt(jwt_token);

    return true;
}

bool SessionClientConnection::handle_make_order_comand(Serialize::TradeRequest& request) {
    Serialize::TradeOrder order = request.order();

    order.set_timestamp(TimeOrderUtils::get_current_timestamp());
    order.set_order_id(TimeOrderUtils::generate_id());

    if (!push_received_from_socket_order_to_queue(order)) {
        spdlog::info("Error to push received from socket order to orders queue : "
                     "user={} order_id={} cost={} amount={} type={}",
                     request.username(), order.order_id(), order.usd_cost(), order.usd_amount(), 
                     (request.order().type() == Serialize::TradeOrder::BUY) ? "BUY" : "SELL");
        return false;
    }
    push_received_from_socket_order_to_active_orders(order);


    auto client_data_manager = session_manager_->get_client_data_manager();
    client_data_manager->notify_order_received();

    return true;
}


void SessionClientConnection::push_received_from_socket_order_to_active_orders(const Serialize::TradeOrder& order) {
    auto client_data_manager = session_manager_->get_client_data_manager();
    client_data_manager->push_order_to_active_orders(order);
}

bool SessionClientConnection::push_received_from_socket_order_to_queue(const Serialize::TradeOrder& order) {
    auto client_data_manager = session_manager_->get_client_data_manager();

    switch (order.type()) {
    case Serialize::TradeOrder::BUY :
        return client_data_manager->push_order_to_order_queue(BUY, order);
    case Serialize::TradeOrder::SELL :
        return client_data_manager->push_order_to_order_queue(SELL, order);
    default:
        return false;
    }
}   

bool SessionClientConnection::handle_view_balance_comand(Serialize::TradeRequest& request, Serialize::TradeResponse& responce) {
    auto client_data_manager = session_manager_->get_client_data_manager();

    try {
        Serialize::AccountBalance account_balance = client_data_manager->get_client_balance(request.username());
        responce.mutable_account_balance()->CopyFrom(account_balance);  
        return true;
    } catch (const std::runtime_error& account_balance_error) {
        spdlog::error("Failed to get client balance: {}", account_balance_error.what());
        return false;
    }
}

void SessionClientConnection::handle_view_all_active_oreders_command(Serialize::TradeRequest& request, Serialize::TradeResponse& responce) {
    auto client_data_manager = session_manager_->get_client_data_manager();

    trade_type_t trade_type = request.command() == (Serialize::TradeRequest::VIEW_ALL_ACTIVE_BUY_ORDERS) ? BUY : SELL;

    Serialize::ActiveOrders all_active_orders = client_data_manager->get_all_active_oreders(trade_type);
    responce.mutable_active_orders()->CopyFrom(all_active_orders);
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
