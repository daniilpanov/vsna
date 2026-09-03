#include "client.h"
#include "helper.h"

void Client::print() const
{
	std::cout << _config.toString() << std::endl;
}

void Client::myPath(const std::vector<std::string>& args) const
{
	std::cout << "Current path: " << _config.getPath() << std::endl;
}

void Client::connect(const std::vector<std::string>& args)
{
	Addr addr;
	if (args.empty())
	{
		addr = _config.getAddr();
	}
	else
	{
		if (args.size() != 1)
		{
			std::cerr << "Usage: connect [ip:port]" << std::endl;
			return;
		}
		auto tempVec = split(args[0], ":");
		if (tempVec.size() != 2)
		{
			std::cerr << "Usage: connect [ip:port]" << std::endl;
			return;
		}
		addr = Addr(tempVec[0], tempVec[1]);
	}

	std::make_shared<ClientSession>(_io_context)
	    ->run(addr.ip().c_str(), addr.port().c_str(), "Hello, World!");
	_io_context.run();
}

void Client::disconnect()
{
	std::cout << "Disconnecting..." << std::endl;
	_io_context.stop();
}
