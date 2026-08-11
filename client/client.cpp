#include "client.h"

void Client::print() const {
    std::cout << _config.toString() << std::endl;
}

void Client::showPath(CONST_ARG_VECTOR args) const {
    std::cout << "Server path: " << _config.getPath() << std::endl;
}

void Client::myPath(CONST_ARG_VECTOR args) const {
    std::cout << "Client path: " << _config.getPath() << std::endl;
}

void Client::sendFiles(CONST_ARG_VECTOR args) {

}

void Client::download(CONST_ARG_VECTOR args) {

}

void Client::connect(CONST_ARG_VECTOR args) {
    try {
        const Addr addr = _config.getAddr();

        socket_ptr sock(new tcp::socket(_io_service));
        tcp::resolver::iterator it = _resolver.resolve(addr.ip(), addr.port());

        boost::asio::connect(*sock, it);
        std::cout << "Connected to " << addr.toString() << std::endl;

        boost::thread(&session, sock).join();
    }
    catch (const std::exception& e) {
        std::cerr << "Connection exception: "
                  << e.what() << '\n';
    }
}
