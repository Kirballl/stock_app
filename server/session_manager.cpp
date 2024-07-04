#include "session_manager.hpp"

SessionManager::SessionManager() : is_running_(true), handle_sessions_mutex_() {
}

void SessionManager::init_database() {
    Config config = read_config("server_config.ini");

    std::string db_config_info = "dbname=" + config.dbname +
                                 " user=" + config.dbuser +
                                 " password=" + config.dbpassword +
                                 " host=" + config.dbhost +
                                 " port=" + std::to_string(config.dbport);

    try {
        database_ = std::make_shared<Database>(db_config_info);
    } catch (const std::exception& e) {
        spdlog::error("Error creating database: exception: {}", e.what());
    }
}

//*INFO for testing
void SessionManager::init_mockdb(std::shared_ptr<IDatabase> mock_db) {
    database_ = mock_db;
}

void SessionManager::init_core() {
    core_ = std::make_shared<Core>(shared_from_this());
    core_->load_all_active_orders_from_db();
}

void SessionManager::init_client_data_manager() {
    client_data_manager_ = std::make_shared<ClientDataManager>(shared_from_this());
    client_data_manager_->initialize_from_database();
}

void SessionManager::init_auth() {
    Config config = read_config("server_config.ini");

    auth_ = std::make_shared<Auth>(config.jwt_secret_key);
}

bool SessionManager::is_runnig() {
    return is_running_.load(std::memory_order_acquire);
}

//*INFO: Here session_manager_thread_ 
void SessionManager::run() {
    while (is_running_) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        //*INFO: Handle active sessions
        try_to_create_new_session_client_connection();
    }
}

bool SessionManager::allowed_to_create_new_connection(){
    std::lock_guard<std::mutex> lock(handle_sessions_mutex_);
    if (stop_new_sessions_) {
        return false;
    } 
    return true;
}

void SessionManager::try_to_create_new_session_client_connection() {
    if (!allowed_to_create_new_connection()) {
        return;
    }

    std::unique_ptr<boost::asio::ip::tcp::socket> new_socket;

    while (new_connections_queue_.try_dequeue(new_socket)) {
        auto new_session_client_connection = std::make_shared<SessionClientConnection>(
                                                    std::move(*new_socket), shared_from_this());

        new_session_client_connection->start();

        std::lock_guard add_new_session_lock_guard(handle_sessions_mutex_);
        clients_sessions_.push_back(new_session_client_connection);

        spdlog::info("Session added for client {}", new_session_client_connection->get_client_endpoint_info());
    }
}

//*INFO Avalible only in main server thread
void SessionManager::add_new_connection(boost::asio::ip::tcp::socket new_client_socket) {
    auto new_socket = std::make_unique<boost::asio::ip::tcp::socket>(std::move(new_client_socket));
    if (!new_connections_queue_.enqueue(std::move(new_socket))) {
        spdlog::error("Failed to add new_socket in new_connections_queue_");
    }
}

std::shared_ptr<ClientDataManager> SessionManager::get_client_data_manager() const {
    return client_data_manager_;
}

std::shared_ptr<IDatabase> SessionManager::get_database() const {
    return database_;
}

std::shared_ptr<Auth> SessionManager::get_auth() const {
    return auth_;
}
std::shared_ptr<Core> SessionManager::get_core() const {
    return core_;
}

bool SessionManager::is_user_logged_in(const std::string& username) {
    std::lock_guard<std::mutex> lock(handle_sessions_mutex_);
    for (const auto& session : clients_sessions_) {
        if (session->get_client_username() == username) {
            return true;
        }
    }
    return false;
}

std::shared_ptr<SessionClientConnection> SessionManager::get_session_by_username(const std::string& username) {
    std::lock_guard<std::mutex> get_session_by_username_lock_guard(handle_sessions_mutex_);
    for (auto& current_session : clients_sessions_) {
        if (current_session->get_client_username() == username) {
            return current_session;
        }
    }

    spdlog::warn("Try to get session by unknown username {}");
    return nullptr;
}


void SessionManager::remove_session(std::shared_ptr<SessionClientConnection> session, std::string client_endpoint_info) {
    std::lock_guard<std::mutex> remove_session_lock_guard(handle_sessions_mutex_);

    auto clients_sessions_iterator = std::find(clients_sessions_.begin(), clients_sessions_.end(), session);
    if (clients_sessions_iterator != clients_sessions_.end()) {
        clients_sessions_.erase(clients_sessions_iterator);
        spdlog::info("Session removed for client {}", client_endpoint_info);
    }
}

void SessionManager::stop_accepting_new_sessions() {
    std::lock_guard<std::mutex> lock(handle_sessions_mutex_);
    stop_new_sessions_ = true;
}

void SessionManager::stop() {
    spdlog::info("Stopping server session namager...");
    is_running_.store(false, std::memory_order_release);

    client_data_manager_->notify_to_stop_matching_orders();

    stop_all_sessions();
    spdlog::info("All sessions in SessionManager stoped");
}

void SessionManager::stop_all_sessions() {
    stop_accepting_new_sessions();

     std::vector<std::shared_ptr<SessionClientConnection>> sessions_copy;
    {
        std::lock_guard<std::mutex> lock(handle_sessions_mutex_);
        sessions_copy = clients_sessions_; //*INFO Copy to avoid iterator invalidation
    }

    for (auto& session : sessions_copy) {
        session->close_this_session();
    }

    {
        std::lock_guard<std::mutex> lock(handle_sessions_mutex_);
        clients_sessions_.clear(); //*INFO clear original
    }
}
