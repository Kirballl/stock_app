#include "user_interface.hpp"

UserInterface::UserInterface(Client& client, std::thread& io_context_thread) :
                             client_(client), io_context_thread_(io_context_thread) {
}

void UserInterface::run() {

    std::cout << "Enter Your username:\n" << std::endl;
    std::string client_name;
    std::cin >> client_name;
    // TODO safety cin client name
    client_.set_username(client_name);

    Serialize::TradeRequest request;
    request.set_username(client_.get_username());
    request.set_command(Serialize::TradeRequest::SIGN_IN);
    request.mutable_sing_in_request()->set_username(client_.get_username());
    client_.send_request_to_stock(request);

    std::cout << "\nWelcome to USD exchange!\n"
                     << std::endl;

    while (true) {
        std::cout << "\nMenu:\n"
                     "1) Make order\n"
                     "2) View my balance\n"
                     "3) Exit\n"
                     << std::endl;

        short menu_option_num;
        std::cin >> menu_option_num;
        switch (menu_option_num) {
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
