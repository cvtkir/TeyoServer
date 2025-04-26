
#include "DataBase.hpp"
#include <iostream>
#include <boost/asio/use_awaitable.hpp>
#include <sodium.h>



static void init_sodium() {
	if (sodium_init() < 0) {
		throw std::runtime_error("Failed to initialize libsodium");
	}
}


DataBase::DataBase(const std::string& conn_str, io_context::executor_type executor, size_t pool_size)
    : executor_(executor), pool_(std::make_unique<ConnectionPool>(conn_str, pool_size)) {
    init_sodium();
}


std::string DataBase::hash_password(const std::string& password) {
	char hashed_password[crypto_pwhash_STRBYTES];
	if (crypto_pwhash_str(hashed_password, password.c_str(), password.size(), 
    crypto_pwhash_OPSLIMIT_INTERACTIVE, crypto_pwhash_MEMLIMIT_INTERACTIVE) != 0) 
    {
		throw std::runtime_error("Failed to hash password");
	}
	return std::string(hashed_password);
}


bool DataBase::DataBase::verify_password(const std::string& password, const std::string& hashed_password) {
	return crypto_pwhash_str_verify(hashed_password.c_str(), password.c_str(), password.length()) == 0;
}


awaitable<pqxx::result> DataBase::execute_query(const std::string& query) {
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


awaitable<bool> DataBase::login_user(const std::string& login, const std::string& password) {
    try {
        std::string query = "SELECT password FROM users WHERE login = '" + pqxx::to_string(login) + "'";
        pqxx::result result = co_await execute_query(query);

        if (result.empty()) {
            co_return false; // Пользователь не найден
        }
        std::string hashed_password = result[0][0].as<std::string>();
        bool password_match = verify_password(password, hashed_password);
        co_return password_match;
    }
    catch (const std::exception& e) {
        std::cerr << "Login error: " << e.what() << std::endl;
        co_return false;
    }
}


awaitable<bool> DataBase::signup_user(const std::string& login, const std::string& password) {
    try {
        // Сначала проверяем, существует ли уже пользователь
        std::string check_query = "SELECT id FROM users WHERE login = '" + pqxx::to_string(login) + "'";
        pqxx::result check_result = co_await execute_query(check_query);

        if (!check_result.empty()) {
            co_return false; // Пользователь уже существует
        }

        // Хешируем пароль
        std::string hashed_password = hash_password(password);

        // Создаем нового пользователя
        std::string insert_query =
            "INSERT INTO users (login, password, username) VALUES (" +
            pqxx::to_string(login) + ", " +
            pqxx::to_string(hashed_password) + ", " +
            pqxx::to_string(login) + ")"; // Используем login как username по умолчанию

        co_await execute_query(insert_query);
        co_return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Signup error: " << e.what() << std::endl;
        co_return false;
    }
}


//awaitable<bool> DataBase::authenticate_user(const std::string& login, const std::string& password) {
//	co_return co_await boost::asio::co_spawn(
//		executor_,
//		[this, login, password]() -> boost::asio::awaitable<bool> {
//			try {
//				pqxx::connection conn(conn_str_);
//				pqxx::work txn(conn);
//				pqxx::result result = txn.exec(
//					pqxx::zview("SELECT id FROM users WHERE login = $1 AND password = $2"),
//					pqxx::params(login, password)
//				);
//				txn.commit();
//				co_return !result.empty();
//			}
//			catch (const std::exception& e) {
//				std::cerr << "Database error: " << e.what() << std::endl;
//				co_return false;
//			}
//		},
//		boost::asio::use_awaitable
//	);
//}

