#pragma once
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <iostream>
#include <thread>
#include <vector>

#include "config.h"
#include "session.h"
#include "helper.h"

using tcp = boost::asio::ip::tcp;
using socket_ptr = boost::shared_ptr<tcp::socket>;

class Server : public std::enable_shared_from_this<Server> {
  public:
	Server() : _io_context(max_threads), _acceptor(_io_context)
	{}
	void run();

	void setConfig(const Config& config)
	{
		_config = config;
	}
	Config getConfig() const
	{
		return _config;
	}

  private:
	Config _config;
	boost::asio::io_context _io_context;
	tcp::acceptor _acceptor;
	std::vector<std::thread> _threads;

	void setup_acceptor();
	void do_accept();
	void on_accept(beast::error_code ec, tcp::socket socket);
};