#include "user_interface.hpp"

UserInterface::UserInterface(Client& client, std::thread& io_context_thread) :
                             client_(client), io_context_thread_(io_context_thread) {
}

void UserInterface::run() {

    auth_menu();

    std::cout << "\nWelcome to USD exchange!\n"
                     << std::endl;

    stock_menu();
}


void UserInterface::auth_menu() {
    while (true) {

        std::string menu_message = "\nNTPro:\n"
                                   "1) Sign up\n"
                                   "2) Sign in\n"
                                   "3) Exit\n";
        short auth_menu_option_num = valid_menu_option_num_choice(menu_message, 1, 3);

        switch (auth_menu_option_num) {
            case 1 : 
                [[fallthrough]];
            case 2 : { 
                std::string action = (auth_menu_option_num == 1) ? "Registration" : "Authorization";
                std::cout << action << ":" << std::endl;

                std::string client_username = get_valid_auth_input(
                    "Enter Your username (one word, max 20 characters):",
                    "Error: Username must be one word without spaces and not exceed 20 characters."
                );

                std::string client_password = get_valid_auth_input(
                    "Enter Your password (one word, max 20 characters):",
                    "Error: Password must be one word without spaces and not exceed 20 characters."
                );

                Serialize::TradeRequest::CommandType command = (auth_menu_option_num == 1) ? 
                    Serialize::TradeRequest::SIGN_UP : Serialize::TradeRequest::SIGN_IN;

                if (perform_auth_request(client_username, client_password, command)) {
                    if (auth_menu_option_num == 1) {
                        std::cout << "Account created successfully. Attempting to sign in..." << std::endl;
                        //*INFO If registration was successful, attempt to sign in
                        if (perform_auth_request(client_username, client_password, Serialize::TradeRequest::SIGN_IN)) {
                            std::cout << "Signed in successfully." << std::endl;
                            return;
                        }
                    } else {
                        std::cout << "Signed in successfully." << std::endl;
                        return;
                    }
                } else {
                    std::cout << "Authentication failed. Please try again." << std::endl;
                }

                break;
            }
            case 3 : {
                client_.close();
                if (io_context_thread_.joinable()) {
                    io_context_thread_.join();
                }
                spdlog::shutdown();
                exit(0);
            }
            default : {
                std::cout << "Unknown menu option\n" << std::endl;
                break;
            }
        }
    }
}

short UserInterface::valid_menu_option_num_choice(const std::string& menu_message, short lower_bound, short upper_bound) {
    std::string input;
    short menu_option_num;

    while (true) {
        std::cout << menu_message << std::endl;
        std::getline(std::cin, input);

        try {
            menu_option_num = std::stoi(input);

            if (menu_option_num >= lower_bound && menu_option_num <= upper_bound) {

                return menu_option_num;  
            } else {
                std::cout << "Please enter a number between "
                          << lower_bound << " and " << upper_bound << "." << std::endl;
            }
        } catch (const std::invalid_argument& e) {
            std::cout << "Invalid input. Please enter a number." << std::endl;
        } catch (const std::out_of_range& e) {
            std::cout << "Input out of range. Please enter a valid menu option." << std::endl;
        }
    }
}

std::string UserInterface::get_valid_auth_input(const std::string& prompt, const std::string& error_message) {
    std::string input;
    while (true) {
        std::cout << prompt << std::endl;
        std::getline(std::cin, input);
        if (input.find(' ') == std::string::npos && input.length() <= MAX_AUTH_CHAR_LENGTH) {
            return input;
        }
        std::cout << error_message << std::endl;
    }
}

bool UserInterface::perform_auth_request(const std::string& username, const std::string& password, Serialize::TradeRequest::CommandType command) {
    Serialize::TradeRequest request;
    request.set_command(command); 

    if (command == Serialize::TradeRequest::SIGN_UP) {
        request.mutable_sign_up_request()->set_username(username);
        request.mutable_sign_up_request()->set_password(password);
    } else if (command == Serialize::TradeRequest::SIGN_IN) {
        request.mutable_sign_in_request()->set_username(username);
        request.mutable_sign_in_request()->set_password(password);
    } else {
        return false;
    }

    if (client_.send_request_to_stock(request)) {
        if (command == Serialize::TradeRequest::SIGN_IN) {
            client_.set_username(username);
        }
        return true;
    }
    return false;
}

void UserInterface::stock_menu() {
    while (true) {
        std::string menu_message = "\nMenu:\n"
                                   "1) Make order\n"
                                   "2) View my balance\n"
                                   "3) View all active orders\n"  
                                   "4) View last completed trades\n"
                                   "5) View qoute histiry\n"
                                   "6) Cancel active order\n"
                                   "7) Exit\n";
        short main_menu_option_num = valid_menu_option_num_choice(menu_message, 1, 7);

        Serialize::TradeRequest trade_request;
        switch (main_menu_option_num) {
                case 1: {
                    std::string menu_order_type_msg = "Enter order type:\n"
                                                      "1) buy $\n"
                                                      "2) sell $\n";
                    short menu_order_type = valid_menu_option_num_choice(menu_order_type_msg, 1, 2);

                    double usd_cost = get_valid_numeric_input<double>("Enter USD cost (RUB)", 0.01, 1000.0);
                    int usd_amount = get_valid_numeric_input<int>("Enter USD amount", 1, 1000000); 

                    trade_type_t trade_type = (menu_order_type == 1) ? BUY : SELL;
                    Serialize::TradeOrder order = client_.form_order(trade_type, usd_cost, usd_amount);
                    
                    trade_request.set_command(Serialize::TradeRequest::MAKE_ORDER);
                    trade_request.mutable_order()->CopyFrom(order);

                    client_.send_request_to_stock(trade_request);
                    break;
                }
                case 2: {
                    trade_request.set_command(Serialize::TradeRequest::VIEW_BALANCE);
                    client_.send_request_to_stock(trade_request);
                    break;
                }
                case 3: {
                    trade_request.set_command(Serialize::TradeRequest::VIEW_ALL_ACTIVE_ORDERS);
                    client_.send_request_to_stock(trade_request);
                    break;
                }
                case 4: {
                    trade_request.set_command(Serialize::TradeRequest::VIEW_COMPLETED_TRADES);
                    client_.send_request_to_stock(trade_request);
                    break;
                }
                case 5: {
                    trade_request.set_command(Serialize::TradeRequest::VIEW_QUOTE_HISTORY);
                    client_.send_request_to_stock(trade_request);
                    break;
                }
                case 6: {
                    trade_request.set_command(Serialize::TradeRequest::CANCEL_ACTIVE_ORDER);

                    // enter oreder id 
                    client_.send_request_to_stock(trade_request);
                    break;
                }
                case 7: {
                    client_.close();
                    if (io_context_thread_.joinable()) {
                        io_context_thread_.join();
                    }
                    return;
                }
                default: {
                    std::cout << "Unknown menu option\n" << std::endl;
                }
        }
    }
}


template<typename T>
T UserInterface::get_valid_numeric_input(const std::string& prompt, T min_value, T max_value) {
    std::string input;
    T value;
    while (true) {
        std::cout << prompt << " (between " << min_value << " and " << max_value << "): ";
        std::getline(std::cin, input);

        try {
            if constexpr (std::is_same_v<T, int>) {
                value = std::stoi(input);
            } else 
            if constexpr (std::is_same_v<T, double>) {
                value = std::stod(input);
            }

            if (value >= min_value && value <= max_value) {
                return value;
            } else {
                std::cout << "Value must be between " << min_value << " and " << max_value << ". Please try again." << std::endl;
            }
        } catch (const std::invalid_argument&) {
            std::cout << "Invalid input. Please enter a number." << std::endl;
        } catch (const std::out_of_range&) {
            std::cout << "Input out of range. Please enter a value between " << min_value << " and " << max_value << "." << std::endl;
        }
    }
}
