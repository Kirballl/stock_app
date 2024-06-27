#ifndef AUTH_HPP
#define AUTH_HPP

#include <string>
#include <jwt-cpp/jwt.h>

class Auth {
public:
    Auth(const std::string& secret_key);

    std::string generate_token(const std::string& username);
    bool verify_token(const std::string& token, const std::string& username);

private:
    std::string secret_key_;
};

#endif // AUTH_HPP