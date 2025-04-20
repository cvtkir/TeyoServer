
#pragma once

#include <boost/asio.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/experimental/awaitable_operators.hpp>
#include <nlohmann/json.hpp>


using namespace boost::asio;
using namespace boost::asio::experimental::awaitable_operators;
using tcp = ip::tcp;
using json = nlohmann::json;

#define DEFAULT_PORT 42001
#define THREADS_NUM 4