#include "addr.h"

Addr::Addr(const std::string& ip, const std::string& port)
{
	setIp(ip);
	setPort(port);
}

void Addr::setIp(const std::string& ip)
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

void Addr::setPort(const std::string& port)
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
		this->_portNum = static_cast<uint16_t>(port_num);
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