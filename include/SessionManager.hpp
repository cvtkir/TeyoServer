
#pragma once

#include "Session.hpp"
#include "DataBase.hpp"

using boost::asio::awaitable;
using boost::asio::ip::tcp;

class SessionManager {
public:
    SessionManager(tcp::acceptor acceptor, std::shared_ptr<DataBase> db);
    awaitable<void> listener();

private:
	std::shared_ptr<DataBase> db_;
    tcp::acceptor acceptor_;
    std::set<std::shared_ptr<Session>> clients_;
};
