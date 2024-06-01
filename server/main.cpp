#include <iostream>

#include <boost/asio.hpp>

#include "server.hpp"
#include "../common/include/common.hpp" 

int main() {
    try {
        /*Это объект, который управляет асинхронными операциями ввода-вывода.
         Он обрабатывает события, связанные с сетевыми операциями, таймерами
         и другими асинхронными задачами. io_context имеет свою собственную 
         очередь задач, которая исполняется в одном или нескольких потоках.*/ 
        boost::asio::io_context io_context;
        
        Server server(io_context);

        io_context.run();
    }
    catch (std::exception& e) {
        std::cerr << "Exeption: " << e.what() << std::endl;
    }
    return 0;
}
