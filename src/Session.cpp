
#include "Session.hpp"
#include "SessionManager.hpp"
#include <iostream>


Session::Session(tcp::socket socket, std::shared_ptr<SessionManager> manager, std::shared_ptr<Database> db)
	: ws_(std::move(socket)), manager_(manager), db_(db) {}

net::awaitable<void> Session::start() {
	std::cout << "in session" << std::endl;
	auto self = shared_from_this();

	try {
		co_await ws_.async_accept(net::use_awaitable);
		std::cout << "ws connection established" << std::endl;

		while (true) {
			std::cout << "in start while()" << std::endl;
			std::string message = co_await read_message();
			if (message.empty()) {
				std::cerr << "Client disconnected" << std::endl;
				break;
			}
			json j;
			if (!try_parse_json(message, j)) continue;

			std::string type = j.value("type", "");

			if (type == "signup") {
				co_await handle_signup(j);

			}
			else if (type == "login") {
				co_await handle_login(j);
			}
			else if (type == "token_login") {
				co_await handle_token_login(j);
			}
			// else if (type == "message") {
			// 	handle_chat_message(j);
			// }
		}
	}
	catch (const std::exception& e) {
		std::cerr << "Client disconnected: " << e.what() << std::endl;
	}
	if (user_id_ != -1) {
		manager_->remove_user(user_id_);
	}
}

// void Session::deliver(std::shared_ptr<std::string> message) {
// 	auto self = shared_from_this();
// 	net::post(ws_.get_executor(), 
//         [this, self, message]() {
//             co_spawn(ws_.get_executor(), 
//                 [this, self, message]() -> net::awaitable<void> {
//                     co_await do_write(*message);
//                 }, 
//                 net::detached);
//         });
// }

net::awaitable<void> Session::do_write(const std::string& message) {
	std::cout << "Sending message: " << message << std::endl;
	
	try {
        co_await ws_.async_write(net::buffer(message), net::use_awaitable);
    }
    catch(const std::exception& e) {
        std::cerr << "Write error: " << e.what() << std::endl;
        throw;
    }
}



net::awaitable<std::string> Session::read_message() {
	try {
        buffer_.clear();
		// buffer_.consume(buffer_.size());
        auto bytes_read = co_await ws_.async_read(buffer_, net::use_awaitable);
        std::string message = beast::buffers_to_string(buffer_.data());

		std::cout << "Received message: " << message << std::endl;
        
		co_return message;
    }
    catch(const beast::system_error& se) {
        if(se.code() != websocket::error::closed) {
            std::cerr << "Read error: " << se.what() << std::endl;
        }
        co_return "";
    }
    catch(const std::exception& e) {
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


// void Session::handle_chat_message(const json& j) {
// 	json response = {
// 			{"user_id", j.value("user_id", -1)},
// 			{"text", j.value("text", "")}
// 	};
// 	std::string response_str = response.dump() + "\n";
// 	auto shared_msg = std::make_shared<std::string>(response_str);
// 	for (auto& client : clients_) {
//         if (client.get() != this) {
//             client->deliver(std::make_shared<std::string>(response_str));
//         }
//     }
// }



net::awaitable<void> Session::handle_login(const json& j) {
	std::string login = j.value("login", "");
	std::string password = j.value("password", "");

	auto auth_result = co_await db_->login_user(login, password);

	json response;
	if (auth_result.success) {
		if (auth_result.user_id != -1) {
			set_user_id(auth_result.user_id);
			manager_->add_user(user_id_, shared_from_this());
			response = { {"type", "login_success"}, {"token", auth_result.token}, 
			{"message", "Login successful"}, {"user_id", auth_result.user_id} };
		} else {
			response = { {"type", "auth_failed"}, {"message", auth_result.error_message} };
		}
	}
	else {
		response = { {"type", "auth_failed"}, {"message", auth_result.error_message} };
	}

	co_await do_write(response.dump());
}


net::awaitable<void> Session::handle_signup(const json& j) {
	std::string login = j.value("login", "");
	std::string password = j.value("password", "");

	
	bool success = co_await db_->signup_user(login, password);

	json response;
	if (success) {
		response = { {"type", "signup_success"}, {"message", "Signup successful"} };
	}
	else {
		response = { {"type", "auth_failed"}, {"message", "Signup failed: user already exists"} };
	}

	co_await do_write(response.dump());
}

net::awaitable<void> Session::handle_token_login(const json& j) {
	std::string token = j.value("token", "");

	auto token_result = co_await db_->validate_token(token);

	json response;
	if (token_result.success) {
		set_user_id(token_result.user_id);
		manager_->add_user(user_id_, shared_from_this());
		response = { {"type", "token_login_success"}, {"message", "Token login successful"}, {"user_id", user_id_} };
	}
	else {
		response = { {"type", "auth_failed"}, {"message", "Invalid token"} };
	}

	co_await do_write(response.dump());
}