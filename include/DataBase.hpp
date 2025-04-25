
#pragma once
#include <pqxx/pqxx>
#include <boost/asio.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>

using namespace boost::asio;

class DataBase {
public:
	DataBase(const std::string& conn_str, io_context::executor_type executor);
	awaitable<bool> authenticate_user(const std::string& login, const std::string& password);
	awaitable<bool> register_user(const std::string& login, const std::string& password);

private:
	std::string conn_str_;
	io_context::executor_type executor_;
};