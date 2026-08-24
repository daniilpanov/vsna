#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/bind/bind.hpp>
#include <boost/smart_ptr.hpp>
#include <iostream>
#include <thread>

#include "helper.h"

using namespace boost::placeholders;
using tcp = boost::asio::ip::tcp;
using socket_ptr = boost::shared_ptr<tcp::socket>;

namespace asio = boost::asio;
namespace beast = boost::beast;
namespace websocket = boost::beast::websocket;

inline void fail(beast::error_code ec, char const *op)
{
	if (ec == asio::error::operation_aborted)
	{
		return;
	}

	std::cerr << op << ": " << ec.message() << "\n";
}