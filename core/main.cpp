#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <iostream>

#if BUILD_SERVER
#include "server_cli.h"
#endif

#if BUILD_CLIENT
#include "client_ui.h"
#endif

int main(int argc, char *argv[])
{
#if BUILD_SERVER
	std::cout << "Built as a server." << std::endl;
	Server server;
	ServerCLI serverCLI(server);
	serverCLI.run(argc, argv);
#endif

#if BUILD_CLIENT
	std::cout << "Built as a client." << std::endl;
	Client client;
	ClientUI clientUI(client);
	clientUI.run(argc, argv);
#endif

	return EXIT_SUCCESS;
}