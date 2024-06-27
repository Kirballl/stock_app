#ifndef USER_INTERFACE_HPP
#define USER_INTERFACE_HPP

#include "client.hpp"
#include <boost/asio.hpp>
#include <thread>

class UserInterface {
public:
    UserInterface(Client& client, std::thread& io_context_thread);

    void run();
    void auth_menu();
    std::string get_valid_input(const std::string& prompt, const std::string& error_message);
    bool perform_auth_request(const std::string& username, const std::string& password, Serialize::TradeRequest::CommandType command);

private:
    Client& client_;
    std::thread& io_context_thread_;
};

#endif // USER_INTERFACE_HPP
