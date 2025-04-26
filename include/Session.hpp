
#pragma once

#include <boost/asio.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/experimental/awaitable_operators.hpp>
#include <nlohmann/json.hpp>
#include <set>
#include <deque>
#include <memory>
#include <string>
#include "DataBase.hpp"

using namespace boost::asio;
using namespace boost::asio::experimental::awaitable_operators;
using tcp = ip::tcp;
using json = nlohmann::json;

class Session : public std::enable_shared_from_this<Session> {
public:
	Session(tcp::socket socket, std::set<std::shared_ptr<Session>>& clients, std::shared_ptr<DataBase> db);
	awaitable<void> start();
	void deliver(std::shared_ptr<std::string> message);
private:
	awaitable<std::string> read_message(boost::asio::streambuf& buffer);
	bool try_parse_json(const std::string& str, json& j);
	//void handle_auth(const json& j, std::shared_ptr<Session> self);
	awaitable<void> handle_signup(const json& j, std::shared_ptr<Session> self);
	awaitable<void> handle_login(const json& j, std::shared_ptr<Session> self);
	void handle_chat_message(const json& j, std::shared_ptr<Session> self);
	awaitable<void> do_write();

	std::shared_ptr<DataBase> db_;
	tcp::socket socket_;
	std::set<std::shared_ptr<Session>>& clients_;
	std::deque<std::string> write_msgs_;
};