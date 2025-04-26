#include "SessionManager.hpp"
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <iostream>

SessionManager::SessionManager(tcp::acceptor acceptor, std::shared_ptr<DataBase> db)
	: acceptor_(std::move(acceptor)), db_(db) {}

awaitable<void> SessionManager::listener() {
	try {
		for (;;) { // Accept new connections
			tcp::socket socket = co_await acceptor_.async_accept(use_awaitable);
			std::cout << "Client connected\n";
			// Create a new session for the connected client
			auto session = std::make_shared<Session>(std::move(socket), clients_, db_);
			co_spawn(acceptor_.get_executor(), session->start(), detached);
		}
	}
	catch (const std::exception& e) {
		std::cerr << "Listener error: " << e.what() << std::endl;
	}
}
