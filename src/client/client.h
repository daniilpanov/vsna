#pragma once
#include <boost/asio.hpp>
#include <iostream>
#include <string>
#include <vector>

#include "config.h"
#include "session.h"
#include "helper.h"

class Client {
  public:
	Client() : _config(), _io_context(), _resolver(_io_context)
	{}

	void setConfig(const Config& config)
	{
		_config = config;
	}
	Config getConfig() const
	{
		return _config;
	}

	void connect(CONST_ARG_VECTOR);
	void print() const;
	void showPath(CONST_ARG_VECTOR) const;
	void myPath(CONST_ARG_VECTOR) const;
	void sendFiles(CONST_ARG_VECTOR);
	void download(CONST_ARG_VECTOR);

  private:
    Config _config;
	boost::asio::io_context _io_context;
	tcp::resolver _resolver;
};
