
#pragma once
#include <unordered_map>
#include <mutex>
#include <memory>

#include "SessionFwd.hpp"
#include "DatabaseManager.hpp"
#include "common_asio.hpp"

class Session; // Forward declaration of Session class

class SessionManager : public std::enable_shared_from_this<SessionManager>{
public:
    SessionManager(net::ip::tcp::acceptor acceptor, std::shared_ptr<Database> db);
    net::awaitable<void> listener();

    void add_user(int user_id, SessionPtr session);
    void remove_user(int user_id);
    SessionPtr get_user(int user_id);
    
private:
	std::shared_ptr<Database> db_;
    net::ip::tcp::acceptor acceptor_;
    std::unordered_map<int, SessionPtr> online_users_;
    std::mutex users_mutex_;
};
