#ifndef SESSION_MANAGER_HPP
#define SESSION_MANAGER_HPP

#include <thread>
#include <vector>
#include <memory>
#include <mutex>
#include <algorithm>

#include <boost/asio.hpp>
#include "concurrentqueue.h" 

#include "order_queue.hpp"
#include "session_client_connection.hpp"

class SessionClientConnection;
class SessionManager : public std::enable_shared_from_this<SessionManager> {
public:
    SessionManager();
    void run();
    void add_new_session(boost::asio::ip::tcp::socket new_client_socket);
    void create_session_client_connection();
    void remove_session(std::shared_ptr<SessionClientConnection> session, std::string client_endpoint_info);
    void stop();

public:
    std::shared_ptr<OrderQueue> buy_orders_queue_;
    std::shared_ptr<OrderQueue> sell_orders_queue_;

private:
    std::atomic<bool> is_rinning_; // atomic to avalible to stop with other thread
    std::vector<std::shared_ptr<SessionClientConnection>> clients_sessions_;
    std::mutex handle_sessions_mutex_;
    
    moodycamel::ConcurrentQueue<std::unique_ptr<boost::asio::ip::tcp::socket>> new_connections_queue_;
};

#endif // SESSION_MANAGER_HPP
