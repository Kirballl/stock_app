#include <iostream>
#include <memory>
#include <csignal>

#include <boost/asio.hpp>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h>

#include "config.hpp"
#include "common.hpp"
#include "client.hpp"
#include "user_interface.hpp"

void signal_handler(int signal) {
    spdlog::info("Interrupt signal received. Shutting down...");
    spdlog::shutdown();
    exit(signal);
}

int main () {
try {
    Config config = read_config("server_config.ini");

    auto rotating_logger = std::make_shared<spdlog::logger>("client_logs", 
        std::make_shared<spdlog::sinks::rotating_file_sink_mt>("build/logs/client_logs.log", LOGS_FILE_SIZE, AMOUNT_OF_ARCHIVED_FILES));
    spdlog::set_default_logger(rotating_logger);
    spdlog::set_level(set_logging_level(config));

    signal(SIGINT, signal_handler);

    boost::asio::io_context io_context;
    boost::asio::ip::tcp::resolver resolver(io_context);
    auto endpoints = resolver.resolve(config.host, std::to_string(config.port));

    Client client(io_context, endpoints);

    spdlog::info("ping ping");

    std::thread io_context_thread([&io_context](){
        io_context.run();
    });

    UserInterface user_interface(client, io_context_thread);
    user_interface.run();

    } catch (std::exception& e) {
        spdlog::error("Exception: {}", e.what());
        std::cerr << "Exeption: " << e.what() << std::endl;
    }
    // gracefull

    return 0;
}
