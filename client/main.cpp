#include <iostream>
#include <boost/asio.hpp>

#include "common.hpp"
#include "client.hpp"

int main () {
    try {
    boost::asio::io_context io_context;

    boost::asio::ip::tcp::resolver resolver(io_context);
    auto endpoints = resolver.resolve(host, std::to_string(port));
    Client client(io_context, endpoints);

    std::thread t([&io_context](){
        io_context.run();
    });

    std::cout << "Write message to server";
    std::string message;
        while (std::getline(std::cin, message)) {
            client.send_message_to_server(message);
        }

    client.close();
    t.join();

    } catch (std::exception& e) {
        std::cerr << "Exeption: " << e.what() << std::endl;
    }

    return 0;
}