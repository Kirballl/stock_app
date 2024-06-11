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
#include "session_manager.hpp"
#include "trade_market_protocol.pb.h"

class SessionManager;

// Handle certain client connection 
class SessionClientConnection : public  std::enable_shared_from_this<SessionClientConnection> {
public:
   SessionClientConnection(boost::asio::ip::tcp::socket socket, 
                           std::shared_ptr<SessionManager> session_manager);

   void start();
   
   std::string get_client_endpoint_info() const;
   bool push_received_from_socket_order_to_queue(const Serialize::TradeOrder& order);
   void close_this_session();

private:
   void async_read_data_from_socket();
   Serialize::TradeOrder convert_raw_data_to_command(std::size_t length);
   Serialize::TradeResponse handle_received_command(Serialize::TradeOrder order);
   void async_write_data_to_socket(const Serialize::TradeResponse& response);

private:
   boost::asio::ip::tcp::socket socket_;
   std::vector<char> raw_data_from_socket_;
   char raw_data_length_from_socket_[sizeof(uint32_t)];

   std::shared_ptr<SessionManager> session_manager_;
};

#endif // SESSION_CLIENT_CONNECTION
