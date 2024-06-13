#ifndef SERVER_HPP
#define SERVER_HPP

#include <iostream>
#include <memory>
#include <thread>

#include <boost/bind/bind.hpp>
#include <boost/asio.hpp>

#include "config.hpp"
#include "common.hpp"
#include "session_manager.hpp"
#include "session_client_connection.hpp"

// Only accept new connections
class Server {
public:
    Server(boost::asio::io_context& io_context, const Config& config);
    void start();
    void stop();

private:
    void accept_new_connection();
    void stock_core();
    void match_orders();

private:
    boost::asio::io_context& io_context_;
    boost::asio::ip::tcp::acceptor acceptor_;

    std::shared_ptr<SessionManager> session_manager_;
    std::thread session_manager_thread_;

    std::thread matching_orders_thread_;
};

#endif // SERVER_HPP
