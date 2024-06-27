#include "user_interface.hpp"

UserInterface::UserInterface(Client& client, std::thread& io_context_thread) :
                             client_(client), io_context_thread_(io_context_thread) {
}

void UserInterface::run() {

    auth_menu();

    std::cout << "\nWelcome to USD exchange!\n"
                     << std::endl;

    while (true) {
        std::cout << "\nMenu:\n"
                     "1) Make order\n"
                     "2) View my balance\n"
                     "3) Exit\n"
                     << std::endl;

        short main_menu_option_num;
        std::cin >> main_menu_option_num;
        switch (main_menu_option_num) {
                case 1: {
                    std::cout << "Enter order type:\n"
                        "1) buy $\n"
                        "2) sell $\n"
                        << std::endl;

                    short menu_order_type;
                    std::cin >> menu_order_type;
                    switch (menu_order_type) {
                    case 1 : {
                        Serialize::TradeOrder order = client_.form_order(BUY);

                        Serialize::TradeRequest trade_request;
                        trade_request.set_username(client_.get_username());
                        trade_request.set_command(Serialize::TradeRequest::MAKE_ORDER);
                        trade_request.mutable_order()->CopyFrom(order);

                        client_.send_request_to_stock(trade_request);
                        break;
                    }
                    case 2 : {
                        Serialize::TradeOrder order = client_.form_order(SELL);

                        Serialize::TradeRequest trade_request;
                        trade_request.set_username(client_.get_username());
                        trade_request.set_command(Serialize::TradeRequest::MAKE_ORDER);
                        trade_request.mutable_order()->CopyFrom(order);

                        client_.send_request_to_stock(trade_request);
                        break;
                    }
                    default:
                        std::cout << "Unknown order type\n" << std::endl;
                        break;
                    }

                    break;
                }
                case 2: {
                    Serialize::TradeRequest trade_request;
                    trade_request.set_username(client_.get_username());
                    trade_request.set_command(Serialize::TradeRequest::VIEW_BALANCE);
                    client_.send_request_to_stock(trade_request);
                    break;
                }
                case 3: {
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

void UserInterface::auth_menu() {
    while (true) {
        std::cout << "\nNTPro:\n"
                     "1) Sign up\n"
                     "2) Sign in\n"
                     "3) Exit\n" << std::endl;

        std::string input;
        std::getline(std::cin, input);

        short auth_menu_option_num;
        try {
            auth_menu_option_num = std::stoi(input);
        } catch (const std::invalid_argument& e) {
            std::cout << "Invalid input. Please enter a number." << std::endl;
            continue;
        } catch (const std::out_of_range& e) {
            std::cout << "Input out of range. Please enter a valid menu option." << std::endl;
            continue;
        }

        switch (auth_menu_option_num) {
            case 1 : 
                [[fallthrough]];
            case 2 : { 
                std::string action = (auth_menu_option_num == 1) ? "Registration" : "Authorization";
                std::cout << action << ":" << std::endl;

                std::string client_username = get_valid_input(
                    "Enter Your username (one word, max 20 characters):",
                    "Error: Username must be one word without spaces and not exceed 20 characters."
                );

                std::string client_password = get_valid_input(
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

std::string UserInterface::get_valid_input(const std::string& prompt, const std::string& error_message) {
    std::string input;
    while (true) {
        std::cout << prompt << std::endl;
        std::getline(std::cin, input);
        if (input.find(' ') == std::string::npos && input.length() <= 20) {
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
