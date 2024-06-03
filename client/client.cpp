#include "client.hpp"

Client::Client(std::string name, boost::asio::io_context& io_context, const boost::asio::ip::tcp::resolver::results_type& endpoints) 
    : name_(name) , socket_(io_context) {
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

    order.set_username(name_);

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

    return order;
}

void Client::get_response_from_stock() {
    char reply[1024];
    boost::system::error_code error;
    std::size_t reply_length = boost::asio::read(socket_, boost::asio::buffer(reply, sizeof(reply)), error);
    if (error) {
        std::cerr << "Failed to read response: " << error.message() << std::endl;
        return;
    }

    Serialize::TradeResponse response;
    response.ParseFromArray(reply, reply_length);
    std::cout << "Response: " << response.message() << std::endl;
}

void Client::write_data_to_socket(std::string& serialized_order) {
    boost::asio::write(socket_, boost::asio::buffer(serialized_order));
}

void Client::send_order_to_stock(const Serialize::TradeOrder& order) {
    std::string serialized_order;
    order.SerializeToString(&serialized_order);

    write_data_to_socket(serialized_order);

    get_response_from_stock();
}

std::string Client::get_name() {
    return name_;
}

void Client::close() {
    socket_.close();
}