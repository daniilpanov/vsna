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

	void connect(const std::vector<std::string>&);
	void print() const;
	void showPath(const std::vector<std::string>&) const;
	void myPath(const std::vector<std::string>&) const;
	void sendFiles(const std::vector<std::string>&);
	void download(const std::vector<std::string>&);
	void sendMsg(const std::vector<std::string>&);
	void disconnect();

  private:
	Config _config;
	boost::asio::io_context _io_context;
};
