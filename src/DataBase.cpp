
#include "DataBase.hpp"
#include <iostream>

DataBase::DataBase(const std::string& conn_str, io_context::executor_type executor)
	: conn_str_(conn_str), executor_(executor) {}


awaitable<bool> DataBase::authenticate_user(const std::string& login, const std::string& password) {
	co_return co_await boost::asio::co_spawn(
		executor_,
		[this, login, password]() -> boost::asio::awaitable<bool> {
			try {
				pqxx::connection conn(conn_str_);
				pqxx::work txn(conn);
				pqxx::result result = txn.exec(
					pqxx::zview("SELECT id FROM users WHERE login = $1 AND password = $2"),
					pqxx::params(login, password)
				);
				txn.commit();
				co_return !result.empty();
			}
			catch (const std::exception& e) {
				std::cerr << "Database error: " << e.what() << std::endl;
				co_return false;
			}
		},
		boost::asio::use_awaitable
	);
}

