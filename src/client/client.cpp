#include "client.h"

void Client::print() const
{
	std::cout << _config.toString() << std::endl;
}

void Client::showPath(CONST_ARG_VECTOR args) const
{
	std::cout << "Server path: " << _config.getPath() << std::endl;
}

void Client::myPath(CONST_ARG_VECTOR args) const
{
	std::cout << "Current path: " << _config.getPath() << std::endl;
}

void Client::sendFiles(CONST_ARG_VECTOR args)
{
	std::cout << "Sending files..." << std::endl;
}

void Client::download(CONST_ARG_VECTOR args)
{
	std::cout << "Downloading..." << std::endl;
}

void Client::connect(CONST_ARG_VECTOR args)
{
	Addr addr = _config.getAddr();
	std::make_shared<ClientSession>(_io_context)
	    ->run(addr.ip().c_str(), addr.port().c_str(), "Hello, World!");
	_io_context.run();
}
