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
	std::cout << "Client path: " << _config.getPath() << std::endl;
}

void Client::sendFiles(CONST_ARG_VECTOR args)
{}

void Client::download(CONST_ARG_VECTOR args)
{}

// void Client::connect(CONST_ARG_VECTOR args) {
//     try {
//         const Addr addr = _config.getAddr();
//         auto results = _resolver.resolve(addr.ip(), addr.port());

//         websocket::stream<tcp::socket> ws(_io_context);
//         boost::asio::connect(ws.next_layer(), results.begin(), results.end());

//         // boost::asio::connect(*sock, results);
//         std::cout << "Connected to " << addr.toString() << std::endl;

//         std::thread(&session, sock).join();
//     }
//     catch (const std::exception& e) {
//         std::cerr << "Connection exception: "
//                   << e.what() << '\n';
//     }
// }

void Client::connect(CONST_ARG_VECTOR args)
{
	Addr addr = _config.getAddr();
	std::make_shared<ClientSession>(_io_context)
	    ->run(addr.ip().c_str(), addr.port().c_str(), "Hello, World!");
	_io_context.run();
}
