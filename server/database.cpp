#include "database.hpp"

const char* Database::CREATE_USERS_TABLE = "CREATE TABLE IF NOT EXISTS users ("
                                            "id SERIAL PRIMARY KEY, "
                                            "username VARCHAR(255) UNIQUE, "
                                            "password VARCHAR(255))";

const char* Database::CREATE_ACTIVE_BUY_ORDERS_TABLE = "CREATE TABLE IF NOT EXISTS active_buy_orders ("
                                                        "id SERIAL PRIMARY KEY, "
                                                        "order_id BIGINT NOT NULL, "
                                                        "username VARCHAR(255), "
                                                        "usd_cost DOUBLE PRECISION, "
                                                        "usd_amount INTEGER, "
                                                        "timestamp TIMESTAMP WITH TIME ZONE)";

const char* Database::CREATE_ACTIVE_SELL_ORDERS_TABLE = "CREATE TABLE IF NOT EXISTS active_sell_orders ("
                                                        "id SERIAL PRIMARY KEY, "
                                                        "order_id BIGINT NOT NULL, "
                                                        "username VARCHAR(255), "
                                                        "usd_cost DOUBLE PRECISION, "
                                                        "usd_amount INTEGER, "
                                                        "timestamp TIMESTAMP WITH TIME ZONE)";

const char* Database::CREATE_CLIENTS_BALANCES_TABLE = "CREATE TABLE IF NOT EXISTS clients_balances ("
                                            "id SERIAL PRIMARY KEY, "
                                            "username VARCHAR(255), "
                                            "usd_balance DOUBLE PRECISION, "
                                            "rub_balance DOUBLE PRECISION)";


const char* Database::CREATE_COMPLETED_ORDERS_TABLE = "CREATE TABLE IF NOT EXISTS completed_orders ("
                                            "id SERIAL PRIMARY KEY, "
                                            "order_id BIGINT NOT NULL, "
                                            "username VARCHAR(255), "
                                            "type VARCHAR(4), "
                                            "usd_cost DOUBLE PRECISION, "
                                            "usd_volume INTEGER, "
                                            "timestamp TIMESTAMP WITH TIME ZONE, "
                                            "completion_timestamp TIMESTAMP WITH TIME ZONE)";


const char* Database::CREATE_QUOTE_HISTORY_TABLE = "CREATE TABLE IF NOT EXISTS quote_history ("
                                                   "id SERIAL PRIMARY KEY, "
                                                   "price DOUBLE PRECISION, "
                                                   "completion_timestamp TIMESTAMP WITH TIME ZONE)";


Database::Database(const std::string& connection_info) : connection_(connection_info) {
    try {
        pqxx::work db_transaction(connection_);

        db_transaction.exec(Database::CREATE_USERS_TABLE);

        //*INFO in core priority_queues
        db_transaction.exec(Database::CREATE_ACTIVE_BUY_ORDERS_TABLE);
        db_transaction.exec(Database::CREATE_ACTIVE_SELL_ORDERS_TABLE);

        //*INFO in client_data_manager
        db_transaction.exec(Database::CREATE_CLIENTS_BALANCES_TABLE);
        db_transaction.exec(Database::CREATE_COMPLETED_ORDERS_TABLE);
        db_transaction.exec(Database::CREATE_QUOTE_HISTORY_TABLE);

        db_transaction.commit();

        spdlog::info("Tables created or already exists.");
    } catch (const pqxx::sql_error& e) {
        spdlog::error("SQL error: {}", e.what());
        spdlog::error("Query was: {}", e.query());
        throw;
    } catch (const std::exception& e) {
        spdlog::error("Error creating database: {}", e.what());
        throw;
    }
}

bool Database::is_user_exists(const std::string& username) {
    std::lock_guard<std::mutex> is_user_exists_lock_guard(mutex_);
    pqxx::work db_transaction(connection_);

    pqxx::result result = db_transaction.exec_params("SELECT 1 FROM users WHERE username = $1", username);
    db_transaction.commit();

    return !result.empty();
}

void Database::add_user(const std::string& username, const std::string& password) {
    std::lock_guard<std::mutex> add_user_lock_guard(mutex_);
    pqxx::work db_transaction(connection_);

    std::string hashed_password = bcrypt::generateHash(password);

    db_transaction.exec_params("INSERT INTO users (username, password)"
                               " VALUES ($1, $2)", username, hashed_password);

    db_transaction.exec_params("INSERT INTO clients_balances (username, usd_balance, rub_balance)"
                               " VALUES ($1, $2, $3)", username, 0.0, 0.0);

    db_transaction.commit();

    spdlog::info("User {} has added to DB", username);
}

bool Database::authenticate_user(const std::string& username, const std::string& password) {
    std::lock_guard<std::mutex> authenticate_user_lock_guard(mutex_);
    pqxx::work db_transaction(connection_);

    pqxx::result result = db_transaction.exec_params("SELECT password FROM users WHERE username = $1", username);
        
    db_transaction.commit();

    if (result.empty()) {
        spdlog::info("User {} not found in database.", username);
        return false;
    }

    std::string hashed_password = result[0]["password"].c_str();

    return bcrypt::validatePassword(password, hashed_password);
}

void Database::save_active_order_to_db(const Serialize::TradeOrder& order) {
    std::lock_guard<std::mutex> save_active_order_to_db_lock_guard(mutex_);
    pqxx::work db_transaction(connection_);

    std::string table_name = (order.type() == Serialize::TradeOrder::BUY) 
                             ? "active_buy_orders" 
                             : "active_sell_orders";

    db_transaction.exec_params("INSERT INTO " + table_name + " (order_id, username, usd_cost, usd_amount, timestamp)"
                               "VALUES ($1, $2, $3, $4, to_timestamp($5 / 1000.0))",
                                order.order_id(),
                                order.username(),
                                order.usd_cost(),
                                order.usd_amount(),
                                order.timestamp());

    db_transaction.commit();
    spdlog::info("Order saved to DB: {} ({})", order.order_id(), table_name);
}

std::vector<Serialize::TradeOrder> Database::load_active_orders_from_db(Serialize::TradeOrder::TradeType type) {
    std::lock_guard<std::mutex> load_active_orders_from_db_lock_guard(mutex_);
    pqxx::work db_transaction(connection_);

    std::string table_name = (type == Serialize::TradeOrder::BUY) 
                             ? "active_buy_orders" 
                             : "active_sell_orders";
    std::string trade_type_to_log = (type == Serialize::TradeOrder::BUY) 
                             ? "buy" 
                             : "sells";

    //*INFO load all orders     
    pqxx::result result = db_transaction.exec_params(
        "SELECT order_id, username, usd_cost, usd_amount, "
        "EXTRACT(EPOCH FROM timestamp) * 1000 AS timestamp "
        "FROM " + table_name);

    std::vector<Serialize::TradeOrder> orders;
    for (const auto& row : result) {
        Serialize::TradeOrder order;
        order.set_order_id(row["order_id"].as<int64_t>());
        order.set_username(row["username"].c_str());
        order.set_usd_cost(row["usd_cost"].as<double>());
        order.set_usd_amount(row["usd_amount"].as<int32_t>());
        int64_t timestamp_ms = static_cast<int64_t>(row["timestamp"].as<double>());
        order.set_timestamp(timestamp_ms);
        order.set_type(type);
        
        orders.push_back(order);
    }

    db_transaction.commit();

    spdlog::info("Active {} orders loaded from {} database table", trade_type_to_log, table_name);

    return orders;
}

void Database::truncate_active_orders_table() {
    try {
        std::lock_guard<std::mutex> truncate_active_orders_table_lock_guard(mutex_);
        pqxx::work db_transaction(connection_);
        //*INFO clear table
        db_transaction.exec("TRUNCATE TABLE active_buy_orders");
        db_transaction.exec("TRUNCATE TABLE active_sell_orders");

        db_transaction.commit();

    } catch (const pqxx::sql_error& e) {
        spdlog::error("SQL error: {}", e.what());
        spdlog::error("Query was: {}", e.query());
        throw;
    } catch (const std::exception& e) {
        spdlog::error("Error creating database: {}", e.what());
        throw;
    }
}


void Database::update_actual_client_balance_in_db(const Serialize::ClientBalance& client_balance) {
    std::lock_guard<std::mutex> update_actual_client_balance_in_db_lock_guard(mutex_);
    pqxx::work db_transaction(connection_);

    pqxx::result result = db_transaction.exec_params(
            "UPDATE clients_balances "
            "SET usd_balance = $1, rub_balance = $2 "
            "WHERE username = $3",
            client_balance.funds().usd_balance(),
            client_balance.funds().rub_balance(), 
            client_balance.username()
        );

    db_transaction.commit();
    spdlog::info("Actual {} balance saved to DB: usd={} rub={}",
        client_balance.username(), client_balance.funds().usd_balance(), client_balance.funds().rub_balance());
}

std::vector<Serialize::ClientBalance> Database::load_clients_balances_from_db() {
    std::lock_guard<std::mutex> load_clients_balances_from_db_lock_guard(mutex_);
    pqxx::work db_transaction(connection_);

    //*INFO load clients balances 
    pqxx::result result = db_transaction.exec_params(
        "SELECT username, usd_balance, rub_balance FROM clients_balances");
    
    std::vector<Serialize::ClientBalance> clients_balances;
    for (const auto& row : result) {
        Serialize::ClientBalance client;
        client.set_username(row["username"].c_str());
        client.mutable_funds()->set_usd_balance(row["usd_balance"].as<double>());
        client.mutable_funds()->set_rub_balance(row["rub_balance"].as<double>());

        clients_balances.push_back(client);
    }

    db_transaction.commit();

    spdlog::info("Clients balances loaded to client_data_manager");

    return clients_balances;
}

void Database::save_completed_order_to_db(const Serialize::TradeOrder& order, int64_t completion_timestamp) {
    std::lock_guard<std::mutex> save_completed_order_to_db_lock_guard(mutex_);
    pqxx::work db_transaction(connection_);

    std::string order_type = order.type() == Serialize::TradeOrder::BUY ? "buy" : "sell";

    db_transaction.exec_params("INSERT INTO completed_orders ("
                               " order_id, "
                               " username, "
                               " type, "
                               " usd_cost, "
                               " usd_volume, "
                               " timestamp, "
                               " completion_timestamp) "
        "VALUES ($1, $2, $3, $4, $5, to_timestamp($6 / 1000.0), to_timestamp($7 / 1000.0))",
                                order.order_id(),
                                order.username(),
                                order_type,
                                order.usd_cost(),
                                order.usd_volume(),
                                order.timestamp(),
                                completion_timestamp);

    db_transaction.commit();
    spdlog::info("Completed order saved to DB: "
                                "order_id = {}, username={}, type={}, usd_cost={}, usd_volume={}, timestamp={}, completion_timestamp={})",
                    order.order_id(), order.username(), order_type, order.usd_cost(), order.usd_volume(), order.timestamp(), completion_timestamp);
}

std::vector<Serialize::TradeOrder> Database::load_last_completed_orders(int number) {
    std::lock_guard<std::mutex> load_last_completed_orders_lock_guard(mutex_);
    pqxx::work db_transaction(connection_);

    //*INFO load last n orders     
    pqxx::result result = db_transaction.exec_params(
        "SELECT id, order_id, username, type, usd_cost, usd_volume, "
        "EXTRACT(EPOCH FROM timestamp) * 1000 AS timestamp "
        "FROM completed_orders "
        "ORDER BY id "
        "DESC LIMIT $1;", number);

    std::vector<Serialize::TradeOrder> orders;
    for (const auto& row : result) {
        Serialize::TradeOrder order;
        order.set_order_id(row["order_id"].as<int64_t>());
        order.set_username(row["username"].c_str());
        std::string type_str = row["type"].as<std::string>();
        if (type_str == "buy") {
            order.set_type(Serialize::TradeOrder::BUY);
        } else {
            order.set_type(Serialize::TradeOrder::SELL);
        }
        order.set_usd_cost(row["usd_cost"].as<double>());
        order.set_usd_volume(row["usd_volume"].as<int32_t>());
        int64_t timestamp_ms = static_cast<int64_t>(row["timestamp"].as<double>());
        order.set_timestamp(timestamp_ms);

        orders.push_back(order);
    }

    db_transaction.commit();

    spdlog::info("Completed orders loaded to client_data_manager");

    return orders;
}

void Database::save_qoute_to_db(const Serialize::Quote& qoute) {
    std::lock_guard<std::mutex> save_qoute_to_db_lock_guard(mutex_);
    pqxx::work db_transaction(connection_);

    db_transaction.exec_params("INSERT INTO quote_history ("
                               "price, "
                               "completion_timestamp) "
                               "VALUES ($1, to_timestamp($2 / 1000.0))",
                                qoute.price(),
                                qoute.timestamp());

    db_transaction.commit();
    spdlog::info("qoute saved to DB: "
                    "price = {}, timestamp={}", qoute.price(), qoute.timestamp());
}

std::vector<Serialize::Quote> Database::load_quote_history(int number) {
    std::lock_guard<std::mutex> load_quote_history_lock_guard(mutex_);
    pqxx::work db_transaction(connection_);
   
    pqxx::result result = db_transaction.exec_params(
        "SELECT id, price, completion_timestamp "
        "FROM quote_history "
        "ORDER BY id "
        "DESC LIMIT $1;", number);

    std::vector<Serialize::Quote> quotes;
    for (const auto& row : result) {
        Serialize::Quote quote;

        quote.set_price(row["price"].as<double>());
        int64_t completion_timestamp_ms = static_cast<int64_t>(row["completion_timestamp"].as<double>());
        quote.set_timestamp(completion_timestamp_ms);
        
        quotes.push_back(quote);
    }

    db_transaction.commit();

    spdlog::info("Quote history loaded to client_data_manager");

    return quotes;
}
