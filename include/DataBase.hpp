
#pragma once
#include <pqxx/pqxx>
#include <boost/asio.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include "ConnectionPool.hpp"

using namespace boost::asio;

class DataBase {
public:
	DataBase(const std::string& conn_str, io_context::executor_type executor, size_t pool_size);
	awaitable<bool> login_user(const std::string& login, const std::string& password);
	awaitable<bool> signup_user(const std::string& login, const std::string& password);

private:
	boost::asio::awaitable<pqxx::result> execute_query(const std::string& query);
	std::string hash_password(const std::string& password);
	bool verify_password(const std::string& password, const std::string& hashed_password);
	std::unique_ptr<ConnectionPool> pool_;
	io_context::executor_type executor_;
};