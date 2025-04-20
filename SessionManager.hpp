
#pragma once

#include "Session.hpp"

using boost::asio::awaitable;
using boost::asio::ip::tcp;

class SessionManager {
public:
    SessionManager(tcp::acceptor acceptor);
    awaitable<void> listener();

private:
    tcp::acceptor acceptor_;
    std::set<std::shared_ptr<Session>> clients_;
};
