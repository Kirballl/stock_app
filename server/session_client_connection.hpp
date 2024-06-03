#ifndef SESSION_CLIENT_CONNECTION
#define SESSION_CLIENT_CONNECTION

#include <iostream>
#include <boost/bind/bind.hpp>
#include <boost/asio.hpp>
#include <memory>

#include "core.hpp"
#include "common.hpp"

// Handle certain client connection 
class SessionClientConnection : public  std::enable_shared_from_this<SessionClientConnection> {
 public:
   SessionClientConnection(boost::asio::ip::tcp::socket socket, Core& core);
   void start();

 private:
   void async_read_data_from_socket();
   void async_write_data_to_socket(const Serialize::TradeResponse& response);

 private:
   boost::asio::ip::tcp::socket socket_;
   enum {max_message_length = 512};
   char data_[max_message_length];

   // trade logic
   Core core_;
};

#endif // SESSION_CLIENT_CONNECTION
