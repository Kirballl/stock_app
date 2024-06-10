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

    spdlog::info("New order formed: user={} cost={} amount={} type={}", 
                 order.username(), order.usd_cost(), order.usd_amount(), 
                 (order.type() == Serialize::TradeOrder::BUY) ? "BUY" : "SELL");

    return order;
}

void Client::get_response_from_stock() {
    boost::asio::streambuf response_buf;
    boost::system::error_code error;

    boost::asio::read(socket_, boost::asio::buffer(data_length_, sizeof(uint32_t)), error);
    if (error) {
        std::cerr << "Failed to read response length from socket: " << error.message() << std::endl;
        return;
    }

    uint32_t msg_length = ntohl(*reinterpret_cast<uint32_t*>(data_length_));
    std::vector<char> response_data(msg_length);

    boost::asio::read(socket_, boost::asio::buffer(response_data), error);
    if (error) {
        std::cerr << "Failed to read respose message from socket: " << error.message() << std::endl;
        return;
    }

    Serialize::TradeResponse response;
    response.ParseFromArray(response_data.data(), msg_length);
    std::cout << "Response: " << response.response_msg() << std::endl;
}

void Client::write_data_to_socket(const std::string& serialized_order) {
    uint32_t msg_length = htonl(static_cast<uint32_t>(serialized_order.size()));

    std::vector<boost::asio::const_buffer> buffers = {
        boost::asio::buffer(&msg_length, sizeof(uint32_t)),
        boost::asio::buffer(serialized_order)
    };

    boost::asio::write(socket_, buffers);
}

void Client::send_order_to_stock(const Serialize::TradeOrder& order) {
    std::string serialized_order;
    order.SerializeToString(&serialized_order);

    std::cout << "write_data_to_socket(serialized_order)" << std::endl;
    write_data_to_socket(serialized_order);

    std::cout << "get_response_from_stock()" << std::endl;
    get_response_from_stock();
}

void Client::close() {
    socket_.close();
}
