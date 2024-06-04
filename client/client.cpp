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
    boost::asio::streambuf response_buf;
    boost::system::error_code error;

    std::size_t reply_length = boost::asio::read_until(socket_, response_buf, "\n", error);
    if (error && error != boost::asio::error::eof) {
        std::cerr << "Failed to read responce: " <<  error.message() << std::endl;
        return;
    }

    std::istream response_stream(&response_buf);
    std::string serialized_response;
    std::getline(response_stream, serialized_response);

    Serialize::TradeResponse response;
    response.ParseFromString(serialized_response);
    std::cout << "Response: " << response.response_msg() << std::endl;
}

void Client::write_data_to_socket(std::string& serialized_order) {
    std::cout << "write_data_to_socket()" << std::endl;
    boost::asio::write(socket_, boost::asio::buffer(serialized_order));
}

void Client::send_order_to_stock(const Serialize::TradeOrder& order) {
    std::string serialized_order;
    order.SerializeToString(&serialized_order);

    write_data_to_socket(serialized_order);

    get_response_from_stock();
}

void Client::close() {
    socket_.close();
}