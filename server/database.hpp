#ifndef DATABASE_HPP
#define DATABASE_HPP

#include <string>

#include <pqxx/pqxx>
#include <spdlog/spdlog.h>
#include "bcrypt.h"

class Database {
public:
    Database(const std::string& connection_info);

    bool is_user_exists(const std::string& username);
    void add_user(const std::string& username, const std::string& password);
    bool authenticate_user(const std::string& username, const std::string& password);

private:
    pqxx::connection connection_;
};

#endif // DATABASE_HPP
