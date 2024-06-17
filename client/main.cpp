#include <iostream>
#include <memory>
#include <csignal>

#include <boost/asio.hpp>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h>

#include "config.hpp"
#include "common.hpp"
#include "client.hpp"

void signal_handler(int signal) {
    spdlog::warn("Interrupt signal ({}) received. Shutting down...", signal);
    spdlog::shutdown();
    exit(signal);
}

int main () {
try {
    auto rotating_logger = std::make_shared<spdlog::logger>("client_logs", 
        std::make_shared<spdlog::sinks::rotating_file_sink_mt>("build/logs/client_logs.log", LOGS_FILE_SIZE, AMOUNT_OF_ARCHIVED_FILES));
    spdlog::set_default_logger(rotating_logger);

    signal(SIGINT, signal_handler);

    Config config = read_config("server_config.ini");

    boost::asio::io_context io_context;

    boost::asio::ip::tcp::resolver resolver(io_context);
    auto endpoints = resolver.resolve(config.host, std::to_string(config.port));

    std::cout << "Enter Your username:\n" << std::endl;
    std::string client_name;
    std::cin >> client_name;

    Client client(client_name, io_context, endpoints);

    spdlog::info("ping ping");

    std::thread io_context_thread([&io_context](){
        io_context.run();
    });

    // Sign in
    Serialize::TradeRequest request;
    request.set_username(client.get_username());
    request.set_command(Serialize::TradeRequest::SIGN_IN);
    client.send_request_to_stock(request);

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
                        Serialize::TradeOrder order = client.form_order(BUY);

                        Serialize::TradeRequest trade_request;
                        trade_request.set_username(client.get_username());
                        trade_request.set_command(Serialize::TradeRequest::MAKE_ORDER);
                        trade_request.mutable_order()->CopyFrom(order);

                        client.send_request_to_stock(trade_request);
                        break;
                    }
                    case 2 : {
                        Serialize::TradeOrder order = client.form_order(SELL);

                        Serialize::TradeRequest trade_request;
                        trade_request.set_username(client.get_username());
                        trade_request.set_command(Serialize::TradeRequest::MAKE_ORDER);
                        trade_request.mutable_order()->CopyFrom(order);

                        client.send_request_to_stock(trade_request);
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
                    trade_request.set_username(client.get_username());
                    trade_request.set_command(Serialize::TradeRequest::VIEW_BALANCE);
                    client.send_request_to_stock(trade_request);
                    break;
                }
                case 3: {
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
        spdlog::error("Exception: {}", e.what());
        std::cerr << "Exeption: " << e.what() << std::endl;
    }
    // gracefull

    return 0;
}
