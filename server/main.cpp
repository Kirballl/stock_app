#include <iostream>
#include <memory>
#include <csignal>

#include <boost/asio.hpp>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h>

#include "config.hpp"
#include "common.hpp" 
#include "server.hpp"

void signal_handler(int signal) {
    spdlog::warn("Interrupt signal ({}) received. Shutting down...", signal);
    spdlog::shutdown();
    exit(signal);
}

int main() {
    try {
        auto rotating_logger = std::make_shared<spdlog::logger>("server_logs",
            std::make_shared<spdlog::sinks::rotating_file_sink_mt>("build/logs/server_logs.log", LOGS_FILE_SIZE, AMOUNT_OF_ARCHIVED_FILES));
        spdlog::set_default_logger(rotating_logger);

        signal(SIGINT, signal_handler);

        Config config = read_config("server_config.ini");

        /*Это объект, который управляет асинхронными операциями ввода-вывода.
         Он обрабатывает события, связанные с сетевыми операциями, таймерами
         и другими асинхронными задачами. io_context имеет свою собственную 
         очередь задач, которая исполняется в одном или нескольких потоках.*/ 
        boost::asio::io_context io_context;
        
        Server server(io_context, config);

        io_context.run();
    }
    catch (std::exception& e) {
        spdlog::error("Exception: {}", e.what());
        std::cerr << "Exeption: " << e.what() << std::endl;
    }
    return 0;
}
