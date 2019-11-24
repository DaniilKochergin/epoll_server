#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/epoll.h>
#include <iostream>
#include <future>
#include "TcpServer.h"


int main(int argc, char const *argv[]) {
    auto serv = TcpServer(8083);
// nc -v localhost 8081
    return 0;
}
