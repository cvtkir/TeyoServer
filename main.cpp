#include <boost/asio.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/thread_pool.hpp>
#include "SessionManager.hpp"
#include <iostream>

#define DEFAULT_PORT 42001
#define THREADS_NUM 4

void initServer() {
	try {
		io_context ctx;
		thread_pool pool(THREADS_NUM);
		co_spawn(
			ctx, []() -> awaitable<void> {
				tcp::acceptor acceptor(co_await this_coro::executor, { tcp::v4(), DEFAULT_PORT });
				SessionManager server(std::move(acceptor));
				co_await server.listener();
			},
			detached
		);

		std::cout << "Server started on port " << DEFAULT_PORT << std::endl;
		post(pool, [&ctx]() { ctx.run(); });
		pool.join();
	}
	catch (std::exception& e) {
		std::cerr << e.what() << std::endl;
	}
}


int main() {
	initServer();
	return 0;
}