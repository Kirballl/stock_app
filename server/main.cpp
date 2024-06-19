#include <iostream>
#include <memory>
#include <csignal>
#include <functional>

#include <boost/asio.hpp>
#include <spdlog/spdlog.h> 
#include <spdlog/sinks/rotating_file_sink.h>

#include "config.hpp"
#include "common.hpp" 
#include "server.hpp"

//TODO gtest - unit tests

void signal_handler(const boost::system::error_code& error, int signal, std::shared_ptr<Server> server) {
    if (!error) {
        server->stop();
        spdlog::info("Interrupt signal ({}) received. Shutting down...", signal);
        spdlog::shutdown();
        exit(signal);
    }
}

int main() {
    try {
        Config config = read_config("server_config.ini");

        auto rotating_logger = std::make_shared<spdlog::logger>("server_logs",
            std::make_shared<spdlog::sinks::rotating_file_sink_mt>("build/logs/server_logs.log", LOGS_FILE_SIZE, AMOUNT_OF_ARCHIVED_FILES));
        spdlog::set_default_logger(rotating_logger);
        spdlog::set_level(set_logging_level(config));

        boost::asio::io_context io_context;
        
        std::shared_ptr<Server> server = std::make_shared<Server>(io_context, config);

        boost::asio::signal_set signals(io_context, SIGINT, SIGTERM);
        signals.async_wait([&server](const boost::system::error_code& error, int signal) {
            signal_handler(error, signal, server);
        });

        io_context.run();
    }
    catch (std::exception& e) {
        spdlog::error("Exception: {}", e.what());
        std::cerr << "Exeption: " << e.what() << std::endl;
    }
    return 0;
}
