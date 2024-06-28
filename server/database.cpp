#include "database.hpp"

Database::Database(const std::string& connection_info) : connection_(connection_info) {
    try {
        pqxx::work db_transaction(connection_);

        db_transaction.exec("CREATE TABLE IF NOT EXISTS users ("
                            "id SERIAL PRIMARY KEY, "
                            "username VARCHAR(255) UNIQUE, "
                            "password VARCHAR(255))");

        db_transaction.exec("CREATE TABLE IF NOT EXISTS active_buy_orders ("
                            "id SERIAL PRIMARY KEY, "
                            "order_id BIGINT NOT NULL, "
                            "username VARCHAR(255), "
                            "usd_cost DOUBLE PRECISION, "
                            "usd_amount INTEGER, "
                            "timestamp TIMESTAMP WITH TIME ZONE)");

        db_transaction.exec("CREATE TABLE IF NOT EXISTS active_sell_orders ("
                            "id SERIAL PRIMARY KEY, "
                            "order_id BIGINT NOT NULL, "
                            "username VARCHAR(255), "
                            "usd_cost DOUBLE PRECISION, "
                            "usd_amount INTEGER, "
                            "timestamp TIMESTAMP WITH TIME ZONE)");

        db_transaction.commit();

        spdlog::info("Table 'users' created or already exists.");
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
    pqxx::work db_transaction(connection_);

    pqxx::result result = db_transaction.exec_params("SELECT 1 FROM users WHERE username = $1", username);
    db_transaction.commit();

    return !result.empty();
}

void Database::add_user(const std::string& username, const std::string& password) {
    pqxx::work db_transaction(connection_);

    std::string hashed_password = bcrypt::generateHash(password);

    db_transaction.exec_params("INSERT INTO users (username, password)"
                               " VALUES ($1, $2)", username, hashed_password);
    db_transaction.commit();

    spdlog::info("User {} has added to DB", username);
}

bool Database::authenticate_user(const std::string& username, const std::string& password) {
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

void Database::save_active_order(const Serialize::TradeOrder& order) {
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

std::vector<Serialize::TradeOrder> Database::load_active_orders(Serialize::TradeOrder::TradeType type) {
    pqxx::work db_transaction(connection_);

    std::cout << " db_transaction.exec_params" << std::endl;

    std::string table_name = (type == Serialize::TradeOrder::BUY) 
                             ? "active_buy_orders" 
                             : "active_sell_orders";

    //*INFO load all orders     
    pqxx::result result = db_transaction.exec_params(
        "SELECT order_id, username, usd_cost, usd_amount, "
        "EXTRACT(EPOCH FROM timestamp) * 1000 AS timestamp "
        "FROM " + table_name);

    std::cout << " const auto& row : result" << std::endl;

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

    //*INFO clear table
    db_transaction.exec("TRUNCATE TABLE " + table_name);

    db_transaction.commit();

    spdlog::info("Loaded to core and cleared {} table in database", table_name);

    return orders;
}
