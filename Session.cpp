
#include "Session.hpp"
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <iostream>


Session::Session(tcp::socket socket, std::set<std::shared_ptr<Session>>& clients)
	: socket_(std::move(socket)), clients_(clients) {}

awaitable<void> Session::start() {
	auto executor = co_await this_coro::executor;
	auto self = shared_from_this();
	clients_.insert(self);

	try {
		boost::asio::streambuf buffer;

		while (true) {
			std::string message = co_await read_message(buffer);
			if (message.empty()) {
				std::cerr << "Client disconnected" << std::endl;
				break;
			}

			json j;
			if (!try_parse_json(message, j)) continue;

			std::string type = j.value("type", "message");

			if (type == "auth") {
				handle_auth(j, self);

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
			co_spawn(socket_.get_executor(), do_write(), detached);
		}
		}
	);
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

void Session::handle_auth(const json& j, std::shared_ptr<Session> self) {
	std::string login = j.value("login", "");
	std::string password = j.value("password", "");

	if (login.empty() || password.empty()) {
		std::cerr << "Invalid auth data" << std::endl;
		return;
	}

	unsigned int assigned_id = static_cast<unsigned int>(reinterpret_cast<uintptr_t>(this) % 10000);

	json response = {
		{"type", "auth_success"},
		{"user_id", assigned_id}
	};

	std::string serialized = response.dump() + "\n";
	auto shared_auth = std::make_shared<std::string>(std::move(serialized));
	self->deliver(shared_auth);

	std::cout << "Authorized user: " << login << std::endl;
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

awaitable<void> Session::do_write() {
	try {
		while (!write_msgs_.empty()) {
			co_await async_write(socket_, buffer(write_msgs_.front()), use_awaitable);
			write_msgs_.pop_front();
		}
	}
	catch (...) {
		clients_.erase(shared_from_this());
	}
	co_return;
}
