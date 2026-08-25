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
	Client() : _config(), _io_context()
	{}

	void setConfig(const Config& config)
	{
		_config = config;
	}
	Config getConfig() const
	{
		return _config;
	}

	void connect(ARG_VECTOR);
	void print() const;
	void showPath(ARG_VECTOR) const;
	void myPath(ARG_VECTOR) const;
	void sendFiles(ARG_VECTOR);
	void download(ARG_VECTOR);
	void sendMsg(ARG_VECTOR);
	void disconnect();

  private:
    Config _config;
	boost::asio::io_context _io_context;
};
