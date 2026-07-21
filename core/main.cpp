#include <iostream>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <asio.hpp>

int main(int argc, char* argv[]) {
    std::cout << "Hello, World!" << std::endl;
    
    #if BUILD_SERVER
    std::cout << "Built as a server." << std::endl;
    #endif

    #if BUILD_CLIENT
    std::cout << "Built as a client." << std::endl; 
    #endif

    return EXIT_SUCCESS;
}