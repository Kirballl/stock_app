#ifndef SESSION_MANAGER_HPP
#define SESSION_MANAGER_HPP

#include <thread>
#include <vector>
#include <memory>
#include <mutex>

#include <boost/asio.hpp>
#include "concurrentqueue.h" 

#include "order_queue.hpp"
#include "session_client_connection.hpp"

class SessionManager {
public:
    SessionManager();
    void run();
    void add_new_session(boost::asio::ip::tcp::socket new_client_socket);
    void start_async_read_wrtite_on_new_connections();
    void stop();

private:
    std::atomic<bool> is_rinning_; // atomic to avalible to stop with other thread
    std::vector<std::shared_ptr<SessionClientConnection>> clients_sessions_;
    std::mutex handle_sessions_mutex_;
    std::shared_ptr<OrderQueue> buy_orders_queue_;
    std::shared_ptr<OrderQueue> sell_orders_queue_;
    moodycamel::ConcurrentQueue<std::unique_ptr<boost::asio::ip::tcp::socket>> new_connections_queue_;
};

#endif // SESSION_MANAGER_HPP
