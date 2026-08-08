#include <iostream>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <asio.hpp>
#include "config.h"

#if BUILD_SERVER
#include "server.h"
#endif

#if BUILD_CLIENT
#include "client.h"
#endif

int main(int argc, char* argv[]) {
    std::string config_path = "config.json";
    Config config = Config::loadFromFile(config_path);    

    #if BUILD_SERVER
    std::cout << "Built as a server." << std::endl;
    start_server(config);
    #endif

    #if BUILD_CLIENT
    std::cout << "Built as a client." << std::endl; 
    
    ClientCLI client;
    client.run(argc, argv);
    #endif

    return EXIT_SUCCESS;
}