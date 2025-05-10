
#include <boost/asio/thread_pool.hpp>
#include <iostream>

#include "common_asio.hpp"
#include "SessionManager.hpp"
#include "DatabaseManager.hpp"

#define DEFAULT_PORT 42001
#define THREADS_NUM 4

void initServer(std::shared_ptr<Database> db) {
	try {
		net::io_context ctx;
		net::thread_pool pool(THREADS_NUM);

		co_spawn(
			ctx, [db]() -> net::awaitable<void> {
				tcp::acceptor acceptor(co_await net::this_coro::executor, { tcp::v4(), DEFAULT_PORT });
				auto manager = std::make_shared<SessionManager>(std::move(acceptor), db);
				co_await manager->listener();
			},
			net::detached
		);

		net::post(pool, [&ctx]() { ctx.run(); });
		std::cout << "Server started on port " << DEFAULT_PORT << std::endl;
		pool.join();
	}
	catch (std::exception& e) {
		std::cerr << e.what() << std::endl;
	}
}


int main() {
	std::string conn_str = "host=localhost port=5432 dbname=messenger_db user=app_user password=kior999";
	net::io_context db_ctx;
	Database db(conn_str, db_ctx.get_executor(), 20);
	auto shared_db = std::make_shared<Database>(std::move(db));
	std::cout << "Initializing server..." << std::endl;
	initServer(shared_db);
	return 0;
}