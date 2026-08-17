#include "config.h"

Config Config::loadFromFile(STRING_ARG filename)
{
	std::ifstream file;
	file.open(filename);
	if (!file.is_open())
	{
		throw std::runtime_error("[!] Cannot open config file: " + filename);
	}

	json j;
	file >> j;
	file.close();

	Config config;

	config.setAddr(Addr(j["ip"].get<std::string>(), j["port"].get<std::string>()));
	config.setPath(j["path"].get<std::string>());

	return config;
}

Config::Config(const Addr& addr, STRING_ARG _server_path)
{
	setAddr(addr);
	setPath(_server_path);
}

void Config::setAddr(const Addr& addr)
{
	this->_addr = addr;
}

void Config::setPath(STRING_ARG _server_path)
{
	if (_server_path.empty())
	{
		throw std::invalid_argument("[!] Server path cannot be empty");
	}

	std::filesystem::path path(_server_path);

	try
	{
		if (!std::filesystem::exists(path))
		{
			throw std::runtime_error("[!] Server path does not exist: " + _server_path);
		}

		if (!std::filesystem::is_directory(path))
		{
			throw std::runtime_error("[!] Server path is not a directory: " + _server_path);
		}

		auto status = std::filesystem::status(path);
		if ((status.permissions() & std::filesystem::perms::owner_read)
		    == std::filesystem::perms::none)
		{
			throw std::runtime_error("[!] Server path is not readable: " + _server_path);
		}

		this->_path = std::filesystem::canonical(path).string();
	}
	catch (const std::filesystem::filesystem_error& e)
	{
		throw std::runtime_error("[!] Filesystem error: " + std::string(e.what()));
	}
}

std::string Config::toString() const
{
	return std::string("[=] Config:\n") + "ADDR: " + this->getAddr().toString() + '\n'
	       + "PATH: " + this->getPath();
}