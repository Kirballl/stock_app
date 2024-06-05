#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <boost/asio.hpp>
#include <iostream>
#include <memory>

#include "trade_market_protocol.pb.h"
#include "common.hpp"

class Client {
 public:
   Client(std::string name, boost::asio::io_context& io_context, const boost::asio::ip::tcp::resolver::results_type& endpoints);

   Serialize::TradeOrder form_order(trade_type_t trade_type);
   void send_order_to_stock(const Serialize::TradeOrder& order);

   void close();

 private:
   void connect_to_server(const boost::asio::ip::tcp::resolver::results_type& endpoints);
   void get_response_from_stock();
   void write_data_to_socket(const std::string& serialized_order);

 private:
   boost::asio::ip::tcp::socket socket_;
   std::string name_;

   std::string message_;
   char data_length_[sizeof(uint32_t)];
};

#endif // CLIENT_HPP
