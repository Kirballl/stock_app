#ifndef SESSION_CLIENT_CONNECTION
#define SESSION_CLIENT_CONNECTION

#include <iostream>
#include <boost/bind/bind.hpp>
#include <boost/asio.hpp>
#include <memory>
#include <vector>
#include <string>

#include "common.hpp"
#include "order_queue.hpp"
#include "trade_market_protocol.pb.h"

// Handle certain client connection 
class SessionClientConnection : public  std::enable_shared_from_this<SessionClientConnection> {
public:
   SessionClientConnection(boost::asio::ip::tcp::socket socket, 
                           std::shared_ptr<OrderQueue> buy_orders_queue,
                           std::shared_ptr<OrderQueue> sell_orders_queue);

   void start();
   void async_read_data_from_socket();
   std::string get_client_endpoint_info() const;
   bool push_received_from_socket_order_to_queue(const Serialize::TradeOrder& order);
   void close();

private:
   void async_write_data_to_socket(const Serialize::TradeResponse& response);

private:
   boost::asio::ip::tcp::socket socket_;
   std::vector<char> raw_data_from_socket_;
   char raw_data_length_from_socket_[sizeof(uint32_t)];

   std::shared_ptr<OrderQueue> buy_orders_queue_;
   std::shared_ptr<OrderQueue> sell_orders_queue_;
   // trade logic
   //Core core_; 
};

#endif // SESSION_CLIENT_CONNECTION
