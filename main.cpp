#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <iostream>
#include "TcpServer.h"

int main(int argc, char const *argv[]) {
    try {
        auto serv = TcpServer(8084);
        serv.BlockingStart();
    } catch (const std::runtime_error &e) {
        std::cout << e.what() << std::endl;
    }
    return 0;
}
