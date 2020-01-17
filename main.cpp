#include <iostream>
#include "TcpServer.h"

int main(int argc, char const *argv[]) {
    try {
        auto io = Context();
        auto serv = TcpServer(&io, 8081);
        io.Start();
    } catch (const std::runtime_error &e) {
        std::cout << e.what() << std::endl;
    }
    return 0;
}
