#ifndef USER_INTERFACE_HPP
#define USER_INTERFACE_HPP

#include "client.hpp"
#include <boost/asio.hpp>
#include <thread>

class UserInterface {
public:
    UserInterface(Client& client, std::thread& io_context_thread);

    void run();

private:
    Client& client_;
    std::thread& io_context_thread_;
};

#endif // USER_INTERFACE_HPP
