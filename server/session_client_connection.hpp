#ifndef SESSION_CLIENT_CONNECTION
#define SESSION_CLIENT_CONNECTION

#include <iostream>
#include <boost/bind/bind.hpp>
#include <boost/asio.hpp>
#include <memory>
#include <vector>

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
   
   std::vector<char> data_;
   char data_length_[sizeof(uint32_t)];

   // trade logic
   Core& core_; 
};

#endif // SESSION_CLIENT_CONNECTION
