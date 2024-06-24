#include "database.hpp"
#include <iostream>

#include <jwt-cpp/jwt.h>


Database::Database(const std::string& connection_info) : connection_(connection_info) {

    std::string password = "top_secret";

    std::string hash = bcrypt::generateHash(password);
}

// void Database::add_user(const std::string& username, const std::string& password) {
//     pqxx::work txn(connection_);
//     std::string hashed_password = BCrypt::generateHash(password);
//     txn.exec_params("INSERT INTO users (username, password) VALUES ($1, $2)", username, hashed_password);
//     txn.commit();
// }

// bool Database::authenticate_user(const std::string& username, const std::string& password) {
//     pqxx::work txn(connection_);
//     pqxx::result result = txn.exec_params("SELECT password FROM users WHERE username = $1", username);
//     txn.commit();
//     if (result.empty()) {
//         return false;
//     }
//     std::string hashed_password = result[0]["password"].c_str();
//     return BCrypt::validatePassword(password, hashed_password);
// }