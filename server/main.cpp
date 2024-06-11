#include <iostream>
#include <memory>
#include <csignal>

#include <boost/asio.hpp>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h>

#include "config.hpp"
#include "common.hpp" 
#include "server.hpp"

std::shared_ptr<Server> server;

void signal_handler(int signal) {
    server->stop();
    spdlog::warn("Interrupt signal ({}) received. Shutting down...", signal);
    spdlog::shutdown();
    exit(signal);
}

int main() {
    try {
        // Using spglog logger
        auto rotating_logger = std::make_shared<spdlog::logger>("server_logs",
            std::make_shared<spdlog::sinks::rotating_file_sink_mt>("build/logs/server_logs.log", LOGS_FILE_SIZE, AMOUNT_OF_ARCHIVED_FILES));
        spdlog::set_default_logger(rotating_logger);

        Config config = read_config("server_config.ini");

        boost::asio::io_context io_context;
        
        server = std::make_shared<Server>(io_context, config);

        signal(SIGINT, signal_handler);

        io_context.run();
    }
    catch (std::exception& e) {
        spdlog::error("Exception: {}", e.what());
        std::cerr << "Exeption: " << e.what() << std::endl;
    }
    return 0;
}
