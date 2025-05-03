#include "SessionManager.hpp"
#include "Session.hpp"
#include <iostream>

SessionManager::SessionManager(tcp::acceptor acceptor, std::shared_ptr<Database> db)
	: acceptor_(std::move(acceptor)), db_(db) {}

net::awaitable<void> SessionManager::listener() {
	try {
		for (;;) { // Accept new connections
			tcp::socket socket = co_await acceptor_.async_accept(net::use_awaitable);
			std::cout << "Client connected\n";
			// Create a new session for the connected client
			auto session = std::make_shared<Session>(std::move(socket), shared_from_this(), db_);
			co_spawn(acceptor_.get_executor(), session->start(), net::detached);
		}
	}
	catch (const std::exception& e) {
		std::cerr << "Listener error: " << e.what() << std::endl;
	}
}

void SessionManager::add_user(int user_id, std::shared_ptr<Session> session) {
	std::lock_guard<std::mutex> lock(users_mutex_);
	online_users_[user_id] = session;
	co_spawn(
		db_->get_executor(),
		[this, user_id]() -> net::awaitable<void> {
			try {
				co_await db_->update_user_status(user_id, "online");
			}
			catch (const std::exception& e) {
				std::cerr << "Error updating user status: " << e.what() << std::endl;
			}
		},
		net::detached
	);
}

void SessionManager::remove_user(int user_id) {
	std::lock_guard<std::mutex> lock(users_mutex_);
    online_users_.erase(user_id);
    
    net::co_spawn(
        db_->get_executor(),
        [this, user_id]() -> net::awaitable<void> {
            try {
                co_await db_->update_user_status(user_id, "offline");
            }
            catch (const std::exception& e) {
                std::cerr << "Failed to update user status: " << e.what() << std::endl;
            }
        },
        net::detached
    );
}

std::shared_ptr<Session> SessionManager::get_user(int user_id) {
	std::lock_guard<std::mutex> lock(users_mutex_);
	auto it = online_users_.find(user_id);
	return it != online_users_.end() ? it->second : nullptr;
}