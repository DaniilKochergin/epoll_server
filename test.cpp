#include <netinet/in.h>
#include <arpa/inet.h>
#include "TcpServer.h"
#include "../gtest/gtest.h"


#define PORT 8091

TEST(correct, spamer) {
    auto serv = TcpServer(PORT);
    serv.AsyncStart();
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    connect(sock, (struct sockaddr *) &addr, sizeof(addr));
    auto point = std::chrono::steady_clock::now();
    int n = 10;
    for (int i = 0; i < n; ++i) {
        std::string s = "beleberda.com";
        (write(sock, s.c_str(), s.size()));
        while (true) {
            char data[1024];
            int rc = read(sock, data, 1024);
            if (rc > 0) {
                std::string d(data);
                std::cout << i << " " << d << " \n";
                break;
            }
        }
    }
    serv.Stop();
}