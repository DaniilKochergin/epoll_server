#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <zconf.h>
#include <arpa/inet.h>
#include <thread>
#include <vector>
#include <fstream>
#include <cassert>
#include <ctime>
#include <cstdlib>

using namespace std;

#define PORT 8083

#define AFK_TIMEOUT 60

static const int ITERATIONS_FOR_STRESS = 10000;


static const string NOT_FOUND = "Name or service not known";
static const string REDIRECT = "Quota is over, retry via";

vector<string> read_data() {
    vector<string> res;
    std::ifstream in("/home/daniil/CLionProjects/untitled12/test_data.txt");
    while (!in.eof()) {
        string s;
        while (getline(in, s)) {
            res.push_back(std::move(s));
        }
    }
    in.close();
    return res;
}


void simple_correct_test() {
    //  cout << "Run simple_correct_test" << endl;
    vector<string> hosts = read_data();
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    assert(connect(sock, (struct sockaddr *) &addr, sizeof(addr)) != -1);
    auto point = std::chrono::steady_clock::now();
    for (const auto &s : hosts) {
        write(sock, s.c_str(), s.size());
        while (true) {
            char data[1024];
            int rc = read(sock, data, 1024);
            if (rc > 0) {
                std::string d(data);
                assert(d.substr(0, REDIRECT.size()) != REDIRECT);
                assert(d.substr(0, NOT_FOUND.size()) != NOT_FOUND);
                break;
            }
        }
    }
}


void many_connections_test() {
    cout << "Run many_connections_test" << endl;
    vector<string> hosts = read_data();
    for (int i = 0; i < 1000; ++i) {
        int sock = socket(AF_INET, SOCK_STREAM, 0);
        struct sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port = htons(PORT);
        addr.sin_addr.s_addr = inet_addr("127.0.0.1");
        assert(connect(sock, (struct sockaddr *) &addr, sizeof(addr)) != -1);
        auto point = std::chrono::steady_clock::now();
        for (const auto &s : hosts) {
            write(sock, s.c_str(), s.size());
            while (true) {
                char data[1024];
                int rc = read(sock, data, 1024);
                if (rc > 0) {
                    std::string d(data);
                    assert(d.substr(0, REDIRECT.size()) != REDIRECT);
                    break;
                }
            }
        }
    }
}


void random_test() {
    // cout << "Run random_test" << endl;
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    assert(connect(sock, (struct sockaddr *) &addr, sizeof(addr)) != -1);
    auto point = std::chrono::steady_clock::now();
    for (int i = 0; i < 10; ++i) {
        string s;
        for (int j = 0; j < 10; ++j) {
            srand(time(nullptr));
            uint8_t c = rand() % 256;
            s.push_back(c);
        }
        write(sock, s.c_str(), s.size());
        while (true) {
            char data[1024];
            int rc = read(sock, data, 1024);
            if (rc > 0) {
                std::string d(data);
                assert(d.substr(0, NOT_FOUND.size()) == NOT_FOUND);
                break;
            }
        }
    }
}

void correct_stress_test() {
    cout << "Run correct_stress_test" << endl;
    vector<string> hosts = read_data();
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    assert(connect(sock, (struct sockaddr *) &addr, sizeof(addr)) != -1);
    for (int i = 0; i < ITERATIONS_FOR_STRESS; ++i) {
        for (const auto &s : hosts) {
            write(sock, s.c_str(), s.size());
            while (true) {
                char data[1024];
                int rc = read(sock, data, 1024);
                if (rc > 0) {
                    std::string d(data);
                    assert(d.substr(0, REDIRECT.size()) != REDIRECT);
                    assert(d.substr(0, NOT_FOUND.size()) != NOT_FOUND);
                    break;
                }
            }
        }
    }
}

void correct_random_stress_test() {
    cout << "Run correct_random_stress_test" << endl;
    for (int i = 0; i < ITERATIONS_FOR_STRESS; ++i) {
        srand(time(0));
        if (rand() % 2 == 0) {
            random_test();
        } else {
            simple_correct_test();
        }
    }
}


int main() {
    cout << "Run simple_correct_test" << endl;
    simple_correct_test();
    cout << "OK" << endl;
    many_connections_test();
    cout << "OK" << endl;
    cout << "Run random_test" << endl;
    random_test();
    cout << "OK" << endl;
    correct_stress_test();
    cout << "OK" << endl;
    correct_random_stress_test();
    cout << "OK" << endl;
    cout << "All test passed! :)" << endl;
    return 0;
}