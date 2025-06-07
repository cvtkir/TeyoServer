
#pragma once

#include <pqxx/pqxx>

#include "common_asio.hpp"
#include "ConnectionPool.hpp"

namespace net = boost::asio;

class Database {
public:
	Database(const std::string& conn_str, net::io_context::executor_type executor, size_t pool_size);

	struct AuthResult {
		bool success;
		int user_id;
		std::string token;
		std::string error_message;
	};

	net::awaitable<AuthResult> login_user(const std::string& login, const std::string& password);
	net::awaitable<bool> signup_user(const std::string& login, const std::string& password);
	net::awaitable<AuthResult> validate_token(const std::string& token);
	net::awaitable<void> update_user_status(int user_id, const std::string& status);
	net::io_context::executor_type get_executor() const { return executor_; }

private:
	boost::asio::awaitable<pqxx::result> execute_query(const std::string& query);
	std::string hash_data(const std::string& data);
	bool verify_hash(const std::string& data, const std::string& hash);
	std::string generate_token(int user_id);

	std::unique_ptr<ConnectionPool> pool_;
	net::io_context::executor_type executor_;
};