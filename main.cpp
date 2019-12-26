#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <iostream>
#include "TcpServer.h"

int main(int argc, char const *argv[]) {
    try {
        auto serv = TcpServer(8082);
        serv.BlockingStart();
    } catch (const std::runtime_error &e) {
        std::cout << e.what() << std::endl;
    }
    //  std::this_thread::sleep_for(std::chrono::seconds(10));
    // serv.Stop();
// nc -v localhost 8081
    return 0;
}
