#include "server.h"

void Server::start() {
    Addr addr = _config.getAddr();
    tcp::acceptor a(_io_service, tcp::endpoint(tcp::v4(), addr.portNum()));

    std::cout << "Addr: " << addr.toString() << std::endl;
    std::cout << "Listening for the connect...\n";
    for (;;) {
        socket_ptr sock(new tcp::socket(_io_service));
        a.accept(*sock);
        std::cout << "Accepted a connect!\n";

        boost::thread(&session, sock).detach();
    } 
}