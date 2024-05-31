#include "client.hpp"

Client::Client(boost::asio::io_context& io_context, const boost::asio::ip::tcp::resolver::results_type& endpoints) 
    : socket_(io_context){
    connect_to_server(endpoints);
}

void Client::connect_to_server(const boost::asio::ip::tcp::resolver::results_type& endpoints) {
    boost::asio::async_connect(socket_, endpoints,
        [this](boost::system::error_code ec, boost::asio::ip::tcp::endpoint) {
            if (!ec) {
                // read_dats_from_socket
            }
    });
}

void Client::async_read_data_from_socket() {
    boost::asio::async_read(socket_,
        boost::asio::buffer(data_, max_message_length),
        [this](boost::system::error_code ec, std::size_t length) {
            if (!ec) {
                std::cout << "Server reply: " << std::string(data_, length) << std::endl;
            }
        });
}

void Client::async_write_data_to_socket() {
    boost::asio::async_write(socket_,
        boost::asio::buffer(message_),
        [this](boost::system::error_code ec, std::size_t length) {
            if (!ec) {
                async_read_data_from_socket();
            }
        }
    );
}

void Client::send_message_to_server(const std::string& message) {
    message_ = message;
    async_write_data_to_socket();
}

void Client::close() {
    socket_.close();
}