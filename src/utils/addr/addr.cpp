#include "addr.h"

Addr::Addr(STRING_ARG ip, STRING_ARG port)
{
	setIp(ip);
	setPort(port);
}

void Addr::setIp(STRING_ARG ip)
{
	if (ip.empty())
	{
		throw std::invalid_argument("[!] IP cannot be empty");
	}

	if (!isValidIPv4(ip))
	{
		throw std::invalid_argument("[!] Invalid IPv4 format: " + ip);
	}

	this->_ip = ip;
}

void Addr::setPort(STRING_ARG port)
{
	if (port.empty())
	{
		throw std::invalid_argument("[!] Port cannot be empty");
	}
	try
	{
		int32_t port_num = std::stoi(port);
		if (port_num < 0 || port_num > 65535)
		{
			throw std::out_of_range("[!] Port out of range (0-65535): " + port);
		}
		this->_port = std::to_string(port_num);
	}
	catch (const std::invalid_argument&)
	{
		throw std::invalid_argument("[!] Invalid port: " + port);
	}
	catch (const std::out_of_range&)
	{
		throw std::out_of_range("[!] Port out of range (0-65535): " + port);
	}
}