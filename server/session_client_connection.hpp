#ifndef SESSION_CLIENT_CONNECTION
#define SESSION_CLIENT_CONNECTION

#include <iostream>
#include <memory>
#include <vector>
#include <string>

#include <boost/bind/bind.hpp>
#include <boost/asio.hpp>
#include <spdlog/spdlog.h>
#include <fmt/core.h>

#include "common.hpp"
#include "time_order_utils.hpp"
#include "order_queue.hpp"
#include "session_manager.hpp"
#include "trade_market_protocol.pb.h"

//*INFO Forward declaration
class SessionManager;

//*INFO Certain client connection 
class SessionClientConnection : public  std::enable_shared_from_this<SessionClientConnection> {
public:
   SessionClientConnection(boost::asio::ip::tcp::socket socket, 
                           std::shared_ptr<SessionManager> session_manager);

   void start();

   std::string get_client_endpoint_info() const;
   std::string get_client_username() const;
   
   void close_this_session();

private:
   void async_read_data_from_socket();

   Serialize::TradeRequest convert_raw_data_to_command(std::size_t length);
   Serialize::TradeResponse handle_received_command(Serialize::TradeRequest& request);

   bool handle_sing_up_command(Serialize::TradeResponse& response, Serialize::TradeRequest& reqest);
   bool handle_sing_in_command(Serialize::TradeResponse& response, Serialize::TradeRequest& request);
   bool handle_make_order_comand(Serialize::TradeRequest& request);
      bool push_received_from_socket_order_to_queue(const Serialize::TradeOrder& order);
      void push_received_from_socket_order_to_active_orders(const Serialize::TradeOrder& order);
   bool handle_view_balance_comand(Serialize::TradeRequest& request, Serialize::TradeResponse& responce);
   void handle_view_all_active_oreders_command(Serialize::TradeRequest& request, Serialize::TradeResponse& responce);

   void async_write_data_to_socket(const Serialize::TradeResponse& response);

private:
   boost::asio::ip::tcp::socket socket_;
   std::vector<char> raw_data_from_socket_;
   char raw_data_length_from_socket_[sizeof(uint32_t)];

   std::string username_;

   std::shared_ptr<SessionManager> session_manager_;
};

#endif // SESSION_CLIENT_CONNECTION
