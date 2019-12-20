#include <iostream>
#include "Communist.h"

void Communist::Add(int address, int fd) {
    if (Quota.count(address) == 0) {
        Quota[address].first = DEFAULT_QUOTA;
        Quota[address].second = 1;
    } else {
        Quota[address].second--;
    }
    FdToUser[fd] = address;
}

void Communist::Remove(int fd) {
    if (Quota[FdToUser[fd]].second == 1) {
        Quota.erase(FdToUser[fd]);
    }
    FdToUser.erase(fd);
}

bool Communist::HaveQuotaToEvent(int fd) {
    return Quota[FdToUser[fd]].first > 0;
}

void Communist::AddQuotaToAll() {
    for (auto &user : Quota) {
        user.second.first += DEFAULT_QUOTA;
    }
}

void Communist::Update(int fd, int64_t time) {
    Quota[FdToUser[fd]].first -= time;
    std::cout<<time<<std::endl;
}

int64_t Communist::GetQuota(int fd) {
    return Quota[FdToUser[fd]].first;
}
