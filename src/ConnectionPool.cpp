
#include "ConnectionPool.hpp"
#include <stdexcept>

ConnectionPool::ConnectionPool(const std::string& conn_str, size_t pool_size)
    : conn_str_(conn_str) {
    for (size_t i = 0; i < pool_size; ++i) {
        auto conn = std::make_shared<pqxx::connection>(conn_str_);
        pool_.push(conn);
    }
}

std::shared_ptr<pqxx::connection> ConnectionPool::get_connection() {
    std::lock_guard<std::mutex> lock(mutex_);

    if (pool_.empty()) {
        return std::make_shared<pqxx::connection>(conn_str_);
    }

    auto conn = pool_.front();
    pool_.pop();
    return conn;
}

void ConnectionPool::release_connection(std::shared_ptr<pqxx::connection> conn) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (conn->is_open()) {
        pool_.push(conn);
    }
    else {
        // Соединение закрыто, создаем новое вместо него
        pool_.push(std::make_shared<pqxx::connection>(conn_str_));
    }
}