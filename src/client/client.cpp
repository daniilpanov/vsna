#include "client.h"
#include "helper.h"


void Client::print() const
{
	std::cout << _config.toString() << std::endl;
}

void Client::showPath(ARG_VECTOR args) const
{
	std::cout << "Server path: " << _config.getPath() << std::endl;
}

void Client::myPath(ARG_VECTOR args) const
{
	std::cout << "Current path: " << _config.getPath() << std::endl;
}

void Client::sendFiles(ARG_VECTOR args)
{
	std::cout << "Sending files..." << std::endl;
}

void Client::download(ARG_VECTOR args)
{
	std::cout << "Downloading..." << std::endl;
}

void Client::connect(ARG_VECTOR args)
{
    Addr addr;
    if (args.empty()) {
        addr = _config.getAddr();
    } else {
        if (args.size() != 1) {
            std::cerr << "Usage: connect [ip:port]" << std::endl;
            return;
        }
        auto tempVec = split(args[0], ':');
        if (tempVec.size() != 2) {
            std::cerr << "Usage: connect [ip:port]" << std::endl;
            return;
        }
        addr = Addr(tempVec[0], tempVec[1]);
    }
	
	std::make_shared<ClientSession>(_io_context)
	    ->run(addr.ip().c_str(), addr.port().c_str(), "Hello, World!");
	_io_context.run();
}

void Client::sendMsg(ARG_VECTOR args)
{
	std::cout << "Sending message..." << std::endl;
	std::string msg = join(args, " ");
	std::cout << "Message: " << msg << std::endl;
	std::cout << "Message length: " << msg.length() << std::endl;
}

void Client::disconnect()
{
	std::cout << "Disconnecting..." << std::endl;
	_io_context.stop();
}
