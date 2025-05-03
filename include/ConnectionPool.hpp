
#pragma once

#include <pqxx/pqxx>
#include <queue>
#include <mutex>
#include <memory>

class ConnectionPool {
public:
    ConnectionPool(const std::string& conn_str, size_t pool_size);

    std::shared_ptr<pqxx::connection> get_connection();
    void release_connection(std::shared_ptr<pqxx::connection> conn);

private:
    std::string conn_str_;
    std::queue<std::shared_ptr<pqxx::connection>> pool_;
    std::mutex mutex_;
};