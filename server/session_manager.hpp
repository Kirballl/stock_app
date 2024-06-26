#ifndef SESSION_MANAGER_HPP
#define SESSION_MANAGER_HPP

#include <thread>
#include <memory>
#include <atomic>
#include <mutex>
#include <vector>
#include <string>

#include <boost/asio.hpp>
#include <spdlog/spdlog.h>

#include "common.hpp"
#include "client_data_manager.hpp"
#include "session_client_connection.hpp"
#include "trade_market_protocol.pb.h"
#include "database.hpp"
#include "auth.hpp"
#include "config.hpp"

//*INFO: Forward declaration
class SessionClientConnection;
class ClientDataManager;

class SessionManager : public std::enable_shared_from_this<SessionManager> {
public:
    SessionManager();

    void run();
    bool is_runnig();

    bool allowed_to_create_new_connection();
    void add_new_connection(boost::asio::ip::tcp::socket new_client_socket);
    void try_to_create_new_session_client_connection();
    void remove_session(std::shared_ptr<SessionClientConnection> session, std::string client_endpoint_info);
    std::shared_ptr<SessionClientConnection> get_session_by_username(const std::string& username);
    
    std::shared_ptr<ClientDataManager> get_client_data_manager() const;
    std::shared_ptr<Database> get_database() const;
    std::shared_ptr<Auth> get_auth() const;

    void stop();
    void stop_all_sessions();
    void stop_accepting_new_sessions();

private:
    bool stop_new_sessions_ = false;
    std::atomic<bool> is_running_;  //*INFO: atomic to avalible to stop with other thread

    std::vector<std::shared_ptr<SessionClientConnection>> clients_sessions_;
    std::mutex handle_sessions_mutex_;

    std::shared_ptr<ClientDataManager> client_data_manager_;
    std::shared_ptr<Database> database_;

    std::shared_ptr<Auth> auth_;
    
    moodycamel::ConcurrentQueue<std::unique_ptr<boost::asio::ip::tcp::socket>> new_connections_queue_;
};

#endif // SESSION_MANAGER_HPP
