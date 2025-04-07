#include <boost/asio.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/asio/experimental/awaitable_operators.hpp>
#include <iostream>
#include <string>
#include <set>
#include <memory>
#include <deque>


using namespace boost::asio;
using namespace boost::asio::experimental::awaitable_operators;
using tcp = ip::tcp;
using namespace std;

#define DEFAULT_PORT 42001
#define BUFFER_SIZE 1024
#define THREADS_NUM 4


class ChatSession : public enable_shared_from_this<ChatSession> {
public:
	ChatSession(tcp::socket socket, set<shared_ptr<ChatSession>>& clients)
		: socket_(std::move(socket)), clients_(clients) {
	}

	awaitable<void> start() {
		auto executor = co_await this_coro::executor;
		auto self = shared_from_this();
		clients_.insert(self);

		try {
			boost::asio::streambuf buffer;

			while (true) {
				std::size_t n = co_await async_read_until(socket_, buffer, '\n', use_awaitable);
				string message{
					buffers_begin(buffer.data()),
					buffers_begin(buffer.data()) + n
				};

				buffer.consume(n);
				auto shared_msg = make_shared<string>(std::move(message));

				for (auto& client : clients_) {
					if (client != self) {
						client->deliver(shared_msg);
					}
				}
			}
		}
		catch (const std::exception& e) {
			std::cerr << "Client disconnected: " << e.what() << std::endl;
			clients_.erase(self);
			co_return;
		}
	}

	void deliver(shared_ptr<string> message) {
		auto self = shared_from_this();
		post(socket_.get_executor(), [this, self, msg = std::move(message)]() {
			bool write_in_progress = !write_msgs_.empty();
			write_msgs_.push_back(std::move(*msg));
			if (!write_in_progress) {
				co_spawn(socket_.get_executor(), do_write(), detached);
			}
			});
	}
private:
	awaitable<void> do_write() {
		try {
			while (!write_msgs_.empty()) {
				co_await async_write(socket_, buffer(write_msgs_.front()), use_awaitable);
				write_msgs_.pop_front();
			}
		}
		catch (...) {
			clients_.erase(shared_from_this());
			co_return;
		}
	}

	tcp::socket socket_;
	set<shared_ptr<ChatSession>>& clients_;
	std::deque<std::string> write_msgs_;
};



class ChatServer
{
public:
	ChatServer(tcp::acceptor acceptor)
		: acceptor_(std::move(acceptor)){}


	awaitable<void> listener() {
		try {
			for (;;) {
				tcp::socket socket = co_await acceptor_.async_accept(use_awaitable);
				std::cout << "Client connected\n";
				auto session = std::make_shared<ChatSession>(std::move(socket), clients_);
				co_spawn(acceptor_.get_executor(), session->start(), detached);
			}
		}
		catch (const std::exception& e) {
			std::cerr << "Accept error: " << e.what() << '\n';
		}
	}
private:
	tcp::acceptor acceptor_;
	std::set<std::shared_ptr<ChatSession>> clients_;
};


int main() {
	try {
		io_context ctx;
		thread_pool pool(THREADS_NUM);
		co_spawn(
			ctx, 
			[]() -> awaitable<void> {
				tcp::acceptor acceptor(co_await this_coro::executor, { tcp::v4(), DEFAULT_PORT });
			ChatServer server(std::move(acceptor));
			co_await server.listener();
			}, 
			detached
		);
		
		cout << "Server started on port " << DEFAULT_PORT << endl;
		post(pool, [&ctx]() { ctx.run(); });
		pool.join();
	}
	catch (exception& e) {
		cerr << e.what() << endl;
	}
}
