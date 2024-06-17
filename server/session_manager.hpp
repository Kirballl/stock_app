#ifndef SESSION_MANAGER_HPP
#define SESSION_MANAGER_HPP

#include <thread>
#include <memory>
#include <atomic>
#include <mutex>
#include <algorithm>
#include <vector>
#include <unordered_map>

#include <boost/asio.hpp>
#include "concurrentqueue.h" 

#include "common.hpp"
#include "order_queue.hpp"
#include "session_client_connection.hpp"

// Forward declaration
class SessionClientConnection;

struct ClientData {
    std::string username;
    double usd_balance;
    double rub_balance;
    std::vector<Serialize::TradeOrder> active_orders;
    std::vector<Serialize::TradeOrder> completed_orders;
};
class SessionManager : public std::enable_shared_from_this<SessionManager> {
public:
    SessionManager();
    void run();
    bool is_runnig();

    void add_new_connection(boost::asio::ip::tcp::socket new_client_socket);
    void try_to_create_new_session_client_connection();
    void remove_session(std::shared_ptr<SessionClientConnection> session, std::string client_endpoint_info);

    void notify_order_received();

    std::shared_ptr<SessionClientConnection> get_session_by_username(const std::string& username);
    
    void change_client_balance(std::string client_jwt, change_balance_type_t change_balance_type, 
                                wallet_type_t wallet_type, double amount);
    Serialize::AccountBalance get_client_balance(std::string client_jwt) const;
    double get_client_balance(std::string client_jwt, wallet_type_t wallet_type) const;

    void stop();

public:
    mutable std::mutex client_data_mutex;
    std::unordered_map<std::string, ClientData> client_data;

    std::condition_variable order_queue_cv;
    std::mutex order_queue_cv_mutex;
    std::shared_ptr<OrderQueue> buy_orders_queue;
    std::shared_ptr<OrderQueue> sell_orders_queue;

private:
    std::atomic<bool> is_running_;  // atomic to avalible to stop with other thread

    std::vector<std::shared_ptr<SessionClientConnection>> clients_sessions_; // hashtable with jwt
    std::mutex handle_sessions_mutex_;
    
    moodycamel::ConcurrentQueue<std::unique_ptr<boost::asio::ip::tcp::socket>> new_connections_queue_;
};

#endif // SESSION_MANAGER_HPP
