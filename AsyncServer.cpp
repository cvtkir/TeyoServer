#include <boost/asio.hpp>
#include <iostream>
#include <string>
#include <set>
#include <memory>
#include <deque>


using boost::asio::ip::tcp;
using namespace std;

#define DEFAULT_PORT 42001
#define BUFFER_SIZE 1024


class ChatSession : public enable_shared_from_this<ChatSession>
{
public:
	ChatSession(tcp::socket socket, set<shared_ptr<ChatSession>>& clients)
		: socket_(std::move(socket)), clients_(clients) {}

	void start() {
		clients_.insert(shared_from_this());
		read_message();
	}

	//void deliver(const string& message) {
	//	boost::asio::async_write(socket_, boost::asio::buffer(message+"\n"),
	//		[](boost::system::error_code, size_t) {});
	//}

	void deliverv2(shared_ptr<string> message) {
		auto self(shared_from_this());
		boost::asio::post(socket_.get_executor(),
			[this, self, msg = std::move(message)]() {
				cout << "deliverv2 lambda" << endl;
				bool write_in_progress = !write_msgs_.empty();
				write_msgs_.push_back(std::move(*msg));
				if (!write_in_progress) {
					do_write();
				}
			});
	}

	void do_write() {
		auto self(shared_from_this());
		boost::asio::async_write(socket_,
			boost::asio::buffer(write_msgs_.front()),
			[this, self](boost::system::error_code ec, size_t /*length*/) {
				if (!ec) {
					write_msgs_.pop_front();
					if (!write_msgs_.empty()) {
						do_write();
					}
				}
				else {
					boost::asio::post(socket_.get_executor(),
						[this, self]() { clients_.erase(self); });
				}
			});
	}

private:
	void read_message() {
		auto self(shared_from_this());
		boost::asio::async_read_until(socket_, buffer_, '\n',
			[this, self](boost::system::error_code ec, size_t length) {
				if (!ec) {
					auto data = buffer_.data();
					string message(
						boost::asio::buffers_begin(data),
						boost::asio::buffers_begin(data) + length);

					buffer_.consume(length);

					auto shared_message = make_shared<string>(std::move(message));

					for (auto& client : clients_) {
						if (client != self) {
							client->deliverv2(shared_message);
							cout << "trying deliverv2" << endl;
						}
					}
					read_message();
				}
				else
				{
					boost::asio::post(socket_.get_executor(), [this, self]()
						{clients_.erase(self); });
				}
			});
	}
	tcp::socket socket_;
	boost::asio::streambuf buffer_;
	set<shared_ptr<ChatSession>>& clients_;
	std::deque<std::string> write_msgs_;
};


class ChatServer
{
public:
	ChatServer(boost::asio::io_context& io_context, short port)
		: acceptor_(io_context, tcp::endpoint(tcp::v4(), port)){
		accept_clients();
	}
private:
	void accept_clients() {
		acceptor_.async_accept(
			[this](boost::system::error_code ec, tcp::socket socket) {
				if (!ec) {
					make_shared<ChatSession>(std::move(socket), clients_)->start();
					cout << "connected client" << endl;
				}
				accept_clients();
			});
	}
	tcp::acceptor acceptor_;
	std::set<std::shared_ptr<ChatSession>> clients_;
};


int main() {
	try {
		boost::asio::io_context io_context;
		ChatServer server(io_context, DEFAULT_PORT);
		io_context.run();
	}
	catch (exception& e) {
		cerr << e.what() << endl;
	}
}
//void handle_client(shared_ptr<tcp::socket> socket)
//{
//	auto buffer = make_shared<boost::asio::streambuf>();
//	buffer->prepare(BUFFER_SIZE);
//
//	boost::asio::async_read_until(*socket, *buffer, "\n",
//		[socket, buffer](const boost::system::error_code& error, size_t bytes_transferred) {
//			if (!error) {
//				string message(boost::asio::buffers_begin(buffer->data()),
//					boost::asio::buffers_begin(buffer->data()) + bytes_transferred);
//				cout << "Received: " << message << endl;
//				boost::asio::async_write(*socket, boost::asio::buffer(message),
//					[](const boost::system::error_code&, std::size_t) {});
//			}
//		});
//}


