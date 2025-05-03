#pragma once

#include <boost/asio.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>

namespace net = boost::asio;  // From <boost/asio.hpp>
using tcp = net::ip::tcp;

// #include <boost/beast.hpp>
// namespace beast = boost::beast; // From <boost/beast.hpp>
// namespace http = beast::http;  // From <boost/beast/http.hpp>
// namespace websocket = beast::websocket; // From <boost/beast/websocket.hpp>