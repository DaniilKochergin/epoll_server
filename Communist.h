#pragma once

#include <unordered_map>
#include <vector>

#define DEFAULT_QUOTA 1000

class Communist {
public:
    void Add(int address, int fd);

    void Update(int fd, int64_t time);

    void Remove(int fd);

    bool HaveQuotaToEvent(int fd);

    void AddQuotaToAll();

    int64_t GetQuota(int fd);

private:
    std::unordered_map<int, int> FdToUser;
    std::unordered_map<int, std::pair<int64_t, int>> Quota;
};