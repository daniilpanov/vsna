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
	std::string port() const
	{
		return std::to_string(_portNum);
	}
	uint16_t portNum() const
	{
		return _portNum;
	}

	void setIp(const std::string&);
	void setPort(const std::string&);

	std::string toString() const
	{
		return _ip + ":" + std::to_string(_portNum);
	}

  private:
	std::string _ip{ "0.0.0.0" };
	uint16_t _portNum{ 5555 };
};
