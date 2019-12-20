#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <iostream>
#include <future>
#include "TcpServer.h"


int main(int argc, char const *argv[]) {
    auto serv = TcpServer(8085);
    serv.Start();
  //  std::this_thread::sleep_for(std::chrono::seconds(10));
  // serv.Stop();
// nc -v localhost 8081
    return 0;
}
