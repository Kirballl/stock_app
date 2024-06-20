#ifndef DATABASE_HPP
#define DATABASE_HPP

#include <pqxx/pqxx>
#include <string>
#include <vector>

class Database {
public:
    Database(const std::string& connection_info);
    ~Database();

    void add_user(const std::string& username, const std::string& password);
    bool authenticate_user(const std::string& username, const std::string& password);

private:
    pqxx::connection connection_;
};

#endif // DATABASE_HPP
