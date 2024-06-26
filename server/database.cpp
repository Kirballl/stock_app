#include "database.hpp"

Database::Database(const std::string& connection_info) : connection_(connection_info) {
    try {
        pqxx::work db_transaction(connection_);
        db_transaction.exec("CREATE TABLE IF NOT EXISTS users ("
                            "id SERIAL PRIMARY KEY, "
                            "username VARCHAR(255) UNIQUE, "
                            "password VARCHAR(255))");
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
