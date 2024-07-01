#include "client.hpp"

Client::Client(boost::asio::io_context& io_context, const boost::asio::ip::tcp::resolver::results_type& endpoints) 
    : socket_(io_context) {

    connect_to_server(endpoints);
}

void Client::connect_to_server(const boost::asio::ip::tcp::resolver::results_type& endpoints) {
    try {
        boost::asio::connect(socket_, endpoints);
    } catch (boost::system::system_error& system_error) {
        std::cout << "Stock is currently closed." << std::endl;
        exit(0);
    }
}

Serialize::TradeOrder Client::form_order(trade_type_t trade_type, double usd_cost, int usd_amount) {
    Serialize::TradeOrder order;

    order.set_usd_cost(usd_cost);
    order.set_usd_amount(usd_amount);
    order.set_usd_volume(usd_amount);

    order.set_username(get_username());

    switch (trade_type) {
        case BUY : {
            order.set_type(Serialize::TradeOrder::BUY);
            break;
        }
        case SELL : {
            order.set_type(Serialize::TradeOrder::SELL);
            break;
        }
    }

    spdlog::info("New order formed: cost={} amount={} type={}", 
                 order.usd_cost(), order.usd_amount(), 
                (order.type() == Serialize::TradeOrder::BUY) ? "BUY" : "SELL");

    return order;
}

bool Client::send_request_to_stock(Serialize::TradeRequest& request) {
    if (request.command() !=  Serialize::TradeRequest::SIGN_UP &&
        request.command() != Serialize::TradeRequest::SIGN_IN) {
        request.set_jwt(jwt_token_);
    }

    std::string serialized_request;
    request.SerializeToString(&serialized_request);

    write_data_to_socket(serialized_request);

    return get_response_from_stock();
}

void Client::write_data_to_socket(const std::string& serialized_request) {
    uint32_t msg_length = htonl(static_cast<uint32_t>(serialized_request.size()));

    std::vector<boost::asio::const_buffer> buffers = {
        boost::asio::buffer(&msg_length, sizeof(uint32_t)),
        boost::asio::buffer(serialized_request)
    };

    boost::asio::write(socket_, buffers);
    spdlog::info("Request has just been sent to the stock");
}

bool Client::get_response_from_stock() {
    boost::asio::streambuf response_buf;
    boost::system::error_code error_code;

    boost::asio::read(socket_, boost::asio::buffer(data_length_, sizeof(uint32_t)), error_code);
    if (error_code) {
        manage_server_socket_error(error_code);
        return false;
    }

    uint32_t msg_length = ntohl(*reinterpret_cast<uint32_t*>(data_length_));
    std::vector<char> response_data(msg_length);

    boost::asio::read(socket_, boost::asio::buffer(response_data), error_code);
    if (error_code) {
        manage_server_socket_error(error_code);
        return false;
    }

    Serialize::TradeResponse response;
    response.ParseFromArray(response_data.data(), msg_length);

    return handle_received_response_from_stock(response);
}

bool Client::handle_received_response_from_stock(const Serialize::TradeResponse& response) {
    switch (response.response_msg()) {
        case Serialize::TradeResponse::SIGN_UP_SUCCESSFUL : {
            return true;
        }
        case Serialize::TradeResponse::USERNAME_ALREADY_TAKEN : {
            std::cout << "\nUsername already taken" << std::endl;
            return false;
        }

        case Serialize::TradeResponse::SIGN_IN_SUCCESSFUL : {
            //*INFO save jwt
            jwt_token_ = response.jwt();
            return true;
        }
        case Serialize::TradeResponse::INVALID_USERNAME_OR_PASSWORD : {
            std::cout << "\nInvalid username or password" << std::endl;
            return false;
        }
        case Serialize::TradeResponse::USER_ALREADY_LOGGED_IN : {
            std::cout << "\nUsername already logged in" << std::endl;
            return false;
        }

        case Serialize::TradeResponse::ORDER_SUCCESSFULLY_CREATED : {
            std::cout << "\nOrder succesfully created" << std::endl;
            return true;
        }

        case Serialize::TradeResponse::SUCCES_VIEW_BALANCE_RESPONCE : {
            std::cout << "\nYour balance: " 
                      << response.account_balance().rub_balance() << " RUB, "
                      << response.account_balance().usd_balance() << " USD."
                      << std::endl;
            return true;
        }   

        case Serialize::TradeResponse::ERROR : {
            std::cout << "\nUnkown error" << std::endl;
            return true;
        }

        default: {
            std::cout << "Error response from stock" << std::endl;
            return false;
        }
    }
}

void Client::manage_server_socket_error(const boost::system::error_code& error_code) {
    if (error_code == boost::asio::error::eof) {
        spdlog::info("Server has closed socket: {}", error_code.message());
    }
    if (error_code == boost::asio::error::connection_reset) {
        spdlog::info("Connection with server was lost: {}", error_code.message());
    }
    std::cout << "Stock has closed unexpectedly" << std::endl;
}

std::string Client::get_username() const {
    return client_username_;
};

void Client::set_username(std::string username) {
    client_username_ = username;
}

void Client::close() {
    socket_.close();
}
