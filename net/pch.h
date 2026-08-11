#include <iostream>
#include <boost/beast/websocket.hpp>
#include <boost/beast/core.hpp>
#include <boost/bind/bind.hpp>
#include <boost/smart_ptr.hpp>
#include <boost/asio.hpp>
#include <thread>
#include "utils.h"


using namespace boost::placeholders;
using tcp = boost::asio::ip::tcp;
using socket_ptr = boost::shared_ptr<tcp::socket>;