#ifndef USER_INTERFACE_HPP
#define USER_INTERFACE_HPP

#include <string>
#include <thread>
#include <limits>

#include <boost/asio.hpp>

#include "client.hpp"

#define MAX_AUTH_CHAR_LENGTH 20

class UserInterface {
public:
    UserInterface(Client& client, std::thread& io_context_thread);

    void run();
    void auth_menu();
    void stock_menu();

    void handle_make_order();
    void handle_view_balance();
    void handle_view_active_orders();
    void handle_view_completed_trades();
    void handle_view_quote_history();
    void handle_cancel_order();
    void handle_exit();

    std::string get_valid_auth_input(const std::string& prompt, const std::string& error_message);
    bool perform_auth_request(const std::string& username, const std::string& password, Serialize::TradeRequest::CommandType command);

    short valid_menu_option_num_choice(const std::string& menu_message, short lower_bound, short upper_bound);
    template<typename T>
    T get_valid_numeric_input(const std::string& prompt, T min_value, T max_value);

private:
    Client& client_;
    std::thread& io_context_thread_;
};

#endif // USER_INTERFACE_HPP
