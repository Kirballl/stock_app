#include <iostream>
#include <boost/asio.hpp>

#include "common.hpp"
#include "client.hpp"


int main () {
    try {
    boost::asio::io_context io_context;

    boost::asio::ip::tcp::resolver resolver(io_context);
    auto endpoints = resolver.resolve(host, std::to_string(port));

    std::cout << "Enter Your name:\n" << std::endl;
    std::string client_name;
    std::cin >> client_name;

    Client client(client_name, io_context, endpoints);

    std::thread io_context_thread([&io_context](){
        io_context.run();
    });

    while (true) {
        std::cout << "Menu:\n"
                     "1) Make order\n"
                     "2) Exit\n"
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
                        Serialize::TradeOrder order = client.form_order(BUY);
                        client.send_order_to_stock(order);
                        break;
                    }
                    case 2 : {
                        Serialize::TradeOrder order = client.form_order(SELL);
                        client.send_order_to_stock(order);
                        break;
                    }
                    default:
                        std::cout << "Unknown order type\n" << std::endl;
                        break;
                    }

                    break;
                }
                case 2: {
                    client.close();
                    io_context_thread.join();
                    exit(0);
                    break;
                }
                default: {
                    std::cout << "Unknown menu option\n" << std::endl;
                }
            }
    }

    } catch (std::exception& e) {
        std::cerr << "Exeption: " << e.what() << std::endl;
    }
    // gracefull

    return 0;
}
