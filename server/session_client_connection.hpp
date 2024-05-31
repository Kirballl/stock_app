#ifndef SESSION_CLIENT_CONNECTION
#define SESSION_CLIENT_CONNECTION

#include <iostream>
#include <boost/bind/bind.hpp>
#include <boost/asio.hpp>
#include <memory>

#include "common.hpp"

// Handle certain client connection 
class Session_client_connection : public  std::enable_shared_from_this<Session_client_connection> {
 public:
    Session_client_connection(boost::asio::ip::tcp::socket socket);
    void start();

 private:
    void async_read_data_from_socket();
    void async_write_data_to_socket(std::size_t length);

 private:
    boost::asio::ip::tcp::socket socket_;
    enum {max_message_length = 512};
    char data_[max_message_length];
};

#endif // SESSION_CLIENT_CONNECTION
