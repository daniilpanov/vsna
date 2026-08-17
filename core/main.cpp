#include <iostream>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio.hpp>

#if BUILD_SERVER
#include "server_cli.h"
#endif

#if BUILD_CLIENT
#include "client_cli.h"
#endif

int main(int argc, char* argv[]) {   
    #if BUILD_SERVER
    std::cout << "Built as a server." << std::endl;
    std::make_shared<Server>()->run();
    #endif

    #if BUILD_CLIENT
    std::cout << "Built as a client." << std::endl; 
    ClientCLI client;
    client.run(argc, argv);
    #endif

    return EXIT_SUCCESS;
}