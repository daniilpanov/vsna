#pragma once
#include <sstream>
#include <string>

#include "helper.h"

class Addr {
  public:
	Addr() = default;
	Addr(const std::string&, const std::string&);

	const std::string& ip() const
	{
		return _ip;
	}
	const std::string& port() const
	{
		return _port;
	}
	uint16_t portNum() const
	{
		return std::stoi(_port);
	}

	void setIp(const std::string&);
	void setPort(const std::string&);

	const std::string toString() const
	{
		return _ip + ":" + _port;
	}

  private:
	std::string _ip{ "0.0.0.0" };
	std::string _port{ "5555" };
};