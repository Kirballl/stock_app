#ifndef SERVER_HPP
#define SERVER_HPP

#include <iostream>
#include <boost/bind/bind.hpp>
#include <boost/asio.hpp>
#include <memory>

#include "common.hpp"
#include "core.hpp"
// Only accept new connections
class Server {
 public:
    Server(boost::asio::io_context& io_context);

 private:
    void accept_new_connection();

 private:
    boost::asio::io_context& io_context_;
    boost::asio::ip::tcp::acceptor acceptor_;
    Core core_;
};

#endif // SERVER_HPP
