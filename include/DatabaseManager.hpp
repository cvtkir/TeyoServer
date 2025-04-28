
#pragma once
#include <pqxx/pqxx>
#include <boost/asio.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include "ConnectionPool.hpp"

using namespace boost::asio;

class Database {
public:
	Database(const std::string& conn_str, io_context::executor_type executor, size_t pool_size);

	struct AuthResult {
		bool success;
		std::string token;
		std::string error_message;
	};

	awaitable<AuthResult> login_user(const std::string& login, const std::string& password);
	awaitable<bool> signup_user(const std::string& login, const std::string& password);
	awaitable<bool> validate_token(const std::string& token);

private:
	boost::asio::awaitable<pqxx::result> execute_query(const std::string& query);
	std::string hash_data(const std::string& data);
	bool verify_hash(const std::string& data, const std::string& hash);
	std::string generate_token(int user_id);

	std::unique_ptr<ConnectionPool> pool_;
	io_context::executor_type executor_;
};