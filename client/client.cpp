#include "client.hpp"

Client::Client(std::string client_username, boost::asio::io_context& io_context, const boost::asio::ip::tcp::resolver::results_type& endpoints) 
    : client_username_(client_username) , socket_(io_context) {
    connect_to_server(endpoints);
}

void Client::connect_to_server(const boost::asio::ip::tcp::resolver::results_type& endpoints) {
    boost::asio::connect(socket_, endpoints);
}

Serialize::TradeOrder Client::form_order(trade_type_t trade_type) {
    Serialize::TradeOrder order;

    std::cout << "Enter USD cost (RUB):\n" << std::endl;
    int usd_cost;
    std::cin >> usd_cost;
    order.set_usd_cost(usd_cost);

    std::cout << "Enter USD amount:\n" << std::endl;
    int usd_amount;
    std::cin >> usd_amount;
    order.set_usd_amount(usd_amount);

    order.set_user_jwt(get_username());

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

void Client::send_request_to_stock(const Serialize::TradeRequest& request) {
    std::string serialized_request;
    request.SerializeToString(&serialized_request);

    write_data_to_socket(serialized_request);

    get_response_from_stock();
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

void Client::get_response_from_stock() {
    boost::asio::streambuf response_buf;
    boost::system::error_code error_code;

    boost::asio::read(socket_, boost::asio::buffer(data_length_, sizeof(uint32_t)), error_code);
    if (error_code) {
        manage_server_socket_error(error_code);
        return;
    }

    uint32_t msg_length = ntohl(*reinterpret_cast<uint32_t*>(data_length_));
    std::vector<char> response_data(msg_length);

    boost::asio::read(socket_, boost::asio::buffer(response_data), error_code);
    if (error_code) {
        manage_server_socket_error(error_code);
        return;
    }

    Serialize::TradeResponse response;
    response.ParseFromArray(response_data.data(), msg_length);

    handle_received_response_from_stock(response);
}

void Client::handle_received_response_from_stock(const Serialize::TradeResponse& response) {
    switch (response.response_msg()) {
        case Serialize::TradeResponse::SIGN_UP_SUCCESSFUL : {
            //std::cout << "\nSusccesfull autorization" << std::endl;
            break;
        }
        case Serialize::TradeResponse::USERNAME_ALREADY_TAKEN : {
            std::cout << "\nUsername already taken" << std::endl;
            break;
        }

        case Serialize::TradeResponse::SIGN_IN_SUCCESSFUL : {
            std::cout << "\nSusccesfull autorization" << std::endl;
            break;
        }
        case Serialize::TradeResponse::INVALID_USERNAME_OR_PASSWORD : {
            std::cout << "\nInvalid username or password" << std::endl;
            break;
        }

        case Serialize::TradeResponse::ORDER_SUCCESSFULLY_CREATED : {
            std::cout << "\nOrder succesfully created" << std::endl;
            break;
        }
        case Serialize::TradeResponse::ERROR_TO_CREATE_ORDER : {
            std::cout << "\nError to create order" << std::endl;
            break;
        }


        case Serialize::TradeResponse::SUCCES_VIEW_BALANCE_RESPONCE : {
            std::cout << "\nYour balance: " 
                      << response.account_balance().rub_balance() << " RUB, "
                      << response.account_balance().usd_balance() << " USD."
                      << std::endl;
            break;
        }    
        case Serialize::TradeResponse::ERROR_VIEW_BALANCE : {
            std::cout << "\nError to view: "  << std::endl;
            break;
        }

        default: {
            std::cout << "Error response from stock" << std::endl;
            break;
        }
    }
}

void Client::manage_server_socket_error(boost::system::error_code& error_code) {
    if (error_code == boost::asio::error::eof) {
        spdlog::warn("Server has closed socket: {}", error_code.message());
    }
    if (error_code == boost::asio::error::connection_reset) {
        spdlog::warn("Connection with server was lost: {}", error_code.message());
    }
    std::cout << "Stock has closed unexpected" << std::endl;
}

std::string Client::get_username() {
    return client_username_;
};

void Client::close() {
    socket_.close();
}
