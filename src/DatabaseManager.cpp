
#include "DatabaseManager.hpp"
#include <iostream>
#include <boost/asio/use_awaitable.hpp>
#include <sodium.h>
#include <iomanip>


static void init_sodium() {
	if (sodium_init() < 0) {
		throw std::runtime_error("Failed to initialize libsodium");
	}
}


Database::Database(const std::string& conn_str, io_context::executor_type executor, size_t pool_size)
    : executor_(executor), pool_(std::make_unique<ConnectionPool>(conn_str, pool_size)) {
    init_sodium();
}

awaitable<pqxx::result> Database::execute_query(const std::string& query) {
    try {
		auto conn = pool_->get_connection();
        pqxx::work txn(*conn);
        pqxx::result result = txn.exec(query);
        txn.commit();
		pool_->release_connection(conn);
        co_return result;
    }
    catch (const std::exception& e) {
        std::cerr << "Database error: " << e.what() << std::endl;
        throw;
    }
}

std::string Database::hash_data(const std::string& data) {
	char hashed_data[crypto_pwhash_STRBYTES];
	if (crypto_pwhash_str(hashed_data, data.c_str(), data.size(),
    crypto_pwhash_OPSLIMIT_INTERACTIVE, crypto_pwhash_MEMLIMIT_INTERACTIVE) != 0) 
    {
		throw std::runtime_error("Failed to hash password");
	}
	return std::string(hashed_data);
}


bool Database::verify_hash(const std::string& data, const std::string& hashed_data) {
	return crypto_pwhash_str_verify(hashed_data.c_str(), data.c_str(), data.length()) == 0;
}


std::string Database::generate_token(int user_id) {
    unsigned char random_bytes[32];
	randombytes_buf(random_bytes, sizeof(random_bytes));

	auto now = std::chrono::system_clock::now();
	auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();

	std::stringstream ss;
	ss << user_id << ":" << timestamp << ":";
    for (unsigned char byte : random_bytes) {
		ss << std::hex << std::setw(2) << std::setfill('0') << (int)byte;
    }
	return ss.str();
}

awaitable<bool> Database::validate_token(const std::string& token) {
    try {
        std::string hashed_token = hash_data(token);

        std::string query =
            "SELECT 1 FROM sessions WHERE token_hash = " + pqxx::to_string(hashed_token) +
            " AND expires_at > NOW()";

        pqxx::result result = co_await execute_query(query);
        co_return !result.empty();
    }
    catch (const std::exception& e) {
        std::cerr << "Token validation error: " << e.what() << std::endl;
        co_return false;
    }
}


awaitable<Database::AuthResult> Database::login_user(const std::string& login, const std::string& password) {
    try {
        std::string query = "SELECT id, password FROM users WHERE login = '" + pqxx::to_string(login) + "'";
        pqxx::result result = co_await execute_query(query);
        if (result.empty()) {
            co_return AuthResult{false, "", "User not found"};
        }
		int user_id = result[0][0].as<int>();
        std::string hashed_password = result[0][1].as<std::string>();

		if (!verify_hash(password, hashed_password)) {
			co_return AuthResult{ false, "", "Invalid password" };
		}

		std::string token = generate_token(user_id);
		std::string hashed_token = hash_data(token);
		auto expires_at = std::chrono::system_clock::now() + std::chrono::days(60);
        std::time_t expires_at_t = std::chrono::system_clock::to_time_t(expires_at);

        std::string insert_token_query =
            "INSERT INTO sessions (user_id, token_hash, expires_at) VALUES (" +
            std::to_string(user_id) + ", '" +
            pqxx::to_string(hashed_token) + ", '" +
            "TO_TIMESTAMP(" + std::to_string(expires_at_t) + ")";
		co_await execute_query(insert_token_query);

        
		co_return AuthResult{ true, token, "" };
    }
    catch (const std::exception& e) {
        std::cerr << "Login error: " << e.what() << std::endl;
        co_return AuthResult{false, "", "Internal server error"};
    }
}


awaitable<bool> Database::signup_user(const std::string& login, const std::string& password) {
    try {
        std::cout << "signup_user in DatabaseManager" << std::endl;
        std::string check_query = "SELECT id FROM users WHERE login = '" + pqxx::to_string(login) + "'";
        pqxx::result check_result = co_await execute_query(check_query);
        if (!check_result.empty()) {
            co_return false;
        }

        std::string hashed_password = hash_data(password);
        
        std::string insert_query =
            "INSERT INTO users (login, password, username) VALUES ('" +
            pqxx::to_string(login) + "', '" +
            pqxx::to_string(hashed_password) + "', '" +
            pqxx::to_string(login) + "')";

        co_await execute_query(insert_query);
        co_return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Signup error: " << e.what() << std::endl;
        co_return false;
    }
}




