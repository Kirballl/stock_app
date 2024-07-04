#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <iostream>
#include <memory>
#include <string>

#include <boost/asio.hpp>
#include <spdlog/spdlog.h> 

#include "trade_market_protocol.pb.h"
#include "common.hpp"

class Client {
public:
   Client(boost::asio::io_context& io_context, const boost::asio::ip::tcp::resolver::results_type& endpoints);

   Serialize::TradeOrder form_order(trade_type_t trade_type, double usd_cost, int usd_amount);
   Serialize::CancelTradeOrder cancel_order(trade_type_t trade_type, int64_t order_id);
   
   bool send_request_to_stock(Serialize::TradeRequest& request);

   std::string get_username() const;
   void set_username(std::string username);

   void close();

private:
   void connect_to_server(const boost::asio::ip::tcp::resolver::results_type& endpoints);
   bool get_response_from_stock();
   void write_data_to_socket(const std::string& serialized_order);
   bool handle_received_response_from_stock(const Serialize::TradeResponse& response);
   void manage_server_socket_error(const boost::system::error_code& error_code);

private:
   boost::asio::ip::tcp::socket socket_;
   std::string client_username_;
   std::string jwt_token_;

   char data_length_[sizeof(uint32_t)];
};

#endif // CLIENT_HPP
