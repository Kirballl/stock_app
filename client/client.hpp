#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <boost/asio.hpp>
#include <iostream>
#include <memory>

class Client {
 public:
    Client(boost::asio::io_context& io_context, const boost::asio::ip::tcp::resolver::results_type& endpoints);

    void send_message_to_server(const std::string& message);
    void close();

 private:
    void connect_to_server(const boost::asio::ip::tcp::resolver::results_type& endpoints);
    void async_read_data_from_socket();
    void async_write_data_to_socket();

 private:
    boost::asio::ip::tcp::socket socket_;
    std::string message_;
    enum {max_message_length = 512};
    char data_[max_message_length];
};

#endif // CLIENT_HPP
