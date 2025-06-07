
#pragma once

#include <boost/beast.hpp>
#include <nlohmann/json.hpp>
#include <memory>
#include <string>

#include "SessionFwd.hpp"
#include "common_asio.hpp"
#include "DatabaseManager.hpp"

class SessionManager; // Forward declaration

namespace beast = boost::beast; // from <boost/beast.hpp>
namespace http = beast::http; // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
using json = nlohmann::json;

class Session : public std::enable_shared_from_this<Session> {
public:
	Session(tcp::socket socket, SessionManagerPtr manager, std::shared_ptr<Database> db);
	net::awaitable<void> start();

	int get_user_id() const { return user_id_; }
	void set_user_id(int user_id) { user_id_ = user_id; }
	// void deliver(std::shared_ptr<std::string> message);
private:
	net::awaitable<std::string> read_message();
	bool try_parse_json(const std::string& str, json& j);
	net::awaitable<void> handle_signup(const json& j);
	net::awaitable<void> handle_login(const json& j);
	net::awaitable<void> handle_token_login(const json& j);
	// void handle_chat_message(const json& j);
	net::awaitable<void> do_write(const std::string& message);

	std::shared_ptr<Database> db_;
	SessionManagerPtr manager_;
	websocket::stream<tcp::socket> ws_;
	beast::flat_buffer buffer_;
	int user_id_ = -1; 
};