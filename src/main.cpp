
#include <boost/asio.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/thread_pool.hpp>
#include <iostream>
#include <pqxx/pqxx>
#include "SessionManager.hpp"
#include "DataBase.hpp"

#define DEFAULT_PORT 42001
#define THREADS_NUM 4

void initServer(DataBase& db) {
	try {
		io_context ctx;
		thread_pool pool(THREADS_NUM);
		co_spawn(
			ctx, [&db]() -> awaitable<void> {
				tcp::acceptor acceptor(co_await this_coro::executor, { tcp::v4(), DEFAULT_PORT });
				SessionManager server(std::move(acceptor), db);
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
	std::string conn_str = "host=localhost port=5432 dbname=messenger_db user=app_user password=kior999";

	io_context db_ctx;
	DataBase db(conn_str, db_ctx.get_executor());

	initServer(db);
	return 0;
}