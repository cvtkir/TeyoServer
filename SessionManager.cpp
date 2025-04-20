#include "SessionManager.hpp"
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <iostream>

SessionManager::SessionManager(tcp::acceptor acceptor)
	: acceptor_(std::move(acceptor)) {}

awaitable<void> SessionManager::listener() {
	try {
		for (;;) {
			tcp::socket socket = co_await acceptor_.async_accept(use_awaitable);
			std::cout << "Client connected\n";
			auto session = std::make_shared<Session>(std::move(socket), clients_);
			co_spawn(acceptor_.get_executor(), session->start(), detached);
		}
	}
	catch (const std::exception& e) {
		std::cerr << "Listener error: " << e.what() << std::endl;
	}
}
