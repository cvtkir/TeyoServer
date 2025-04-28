
#include "Session.hpp"
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <iostream>


Session::Session(tcp::socket socket, std::set<std::shared_ptr<Session>>& clients, std::shared_ptr<Database> db)
	: socket_(std::move(socket)), clients_(clients), db_(db) {}

awaitable<void> Session::start() {
	auto executor = co_await this_coro::executor;
	auto self = shared_from_this();
	clients_.insert(self);

	try {
		boost::asio::streambuf buffer;
		std::cout << "in start" << std::endl;
		while (true) {
			std::string message = co_await read_message(buffer);
			if (message.empty()) {
				std::cerr << "Client disconnected" << std::endl;
				break;
			}

			json j;
			if (!try_parse_json(message, j)) continue;

			std::string type = j.value("type", "message");

			if (type == "signup") {
				std::cout << "launch signup in session" << std::endl;
				co_await handle_signup(j, self);

			}
			else if (type == "login") {
				co_await handle_login(j, self);
			}
			else if (type == "message") {
				handle_chat_message(j, self);
			}
		}
		clients_.erase(self);
		co_return;
	}
	catch (const std::exception& e) {
		std::cerr << "Client disconnected: " << e.what() << std::endl;
		clients_.erase(self);
		co_return;
	}
}

void Session::deliver(std::shared_ptr<std::string> message) {
	auto self = shared_from_this();
	post(socket_.get_executor(), [this, self, msg = std::move(message)]() {
		bool write_in_progress = !write_msgs_.empty();
		write_msgs_.push_back(std::move(*msg));
		if (!write_in_progress) {
			std::cout << "before co spawn do_write" << std::endl;
			co_spawn(socket_.get_executor(), do_write(), detached);
		}
		}
	);
}

awaitable<void> Session::do_write() {
	try {
		std::cout << "in do_write" << std::endl;
		while (!write_msgs_.empty()) {
			co_await async_write(socket_, buffer(write_msgs_.front()), use_awaitable);
			std::cout << "in do_write while" << std::endl;
			write_msgs_.pop_front();
		}
	}
	catch (...) {
		clients_.erase(shared_from_this());
	}
	co_return;
}



awaitable<std::string> Session::read_message(boost::asio::streambuf& buffer) {
	std::string message;
	try {
		std::size_t n = co_await async_read_until(socket_, buffer, '\n', use_awaitable);
		message = std::string{ buffers_begin(buffer.data()), buffers_begin(buffer.data()) + n };
		buffer.consume(n);
		co_return message;
	}
	catch(const std::exception& e){
		std::cerr << "Read error: " << e.what() << std::endl;
		co_return "";
	}
}

bool Session::try_parse_json(const std::string& str, json& j) {
	try {
		j = json::parse(str);
		return true;
	}
	catch (const json::parse_error& e) {
		std::cerr << "JSON parse error" << e.what() << std::endl;
		return false;
	}
}


void Session::handle_chat_message(const json& j, std::shared_ptr<Session> self) {
	json response = {
			{"user_id", j.value("user_id", -1)},
			{"text", j.value("text", "")}
	};
	std::string response_str = response.dump() + "\n";
	auto shared_msg = std::make_shared<std::string>(response_str);
	for (auto& client : clients_) {
		if (client != self) {
			client->deliver(shared_msg);
		}
	}
}



awaitable<void> Session::handle_login(const json& j, std::shared_ptr<Session> self) {
	std::string login = j.value("login", "");
	std::string password = j.value("password", "");

	auto auth_result = co_await db_->login_user(login, password);

	json response;
	if (auth_result.success) {
		response = { {"type", "auth_success"}, {"token", auth_result.token}, {"message", "Login successful"}};
	}
	else {
		response = { {"type", "auth_failed"}, {"message", "Invalid login or password"} };
	}

	std::string serialized = response.dump() + "\n";
	auto shared_resp = std::make_shared<std::string>(std::move(serialized));
	self->deliver(shared_resp);
}


awaitable<void> Session::handle_signup(const json& j, std::shared_ptr<Session> self) {
	std::cout << "handle_signup in session" << std::endl;
	std::string login = j.value("login", "");
	std::string password = j.value("password", "");

	
	bool success = co_await db_->signup_user(login, password);
	std::cout << "exited from DatabaseManager with:" << success << std::endl;


	json response;
	if (success) {
		response = { {"type", "auth_success"}, {"message", "Signup successful"} };
	}
	else {
		response = { {"type", "auth_failed"}, {"message", "Signup failed: user already exists"} };
	}

	std::string serialized = response.dump() + "\n";
	auto shared_resp = std::make_shared<std::string>(serialized);
	std::cout << "before deliver" << std::endl;
	self->deliver(shared_resp);
}