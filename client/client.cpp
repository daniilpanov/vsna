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
        Addr addr = _config.getAddr();
        tcp::resolver::iterator it = _resolver.resolve(addr.ip(), addr.port());

        boost::asio::connect(_socket, it);

        for (;;) {
            std::cout << "Enter message: ";
            char request[max_length];
            std::cin.getline(request, max_length);
            size_t request_length = strlen(request);
            boost::asio::write(_socket, boost::asio::buffer(request, request_length));

            char reply[max_length];
            size_t reply_length = boost::asio::read(_socket,
                boost::asio::buffer(reply, request_length));
            std::cout << "Reply is: ";
            std::cout.write(reply, reply_length);
            std::cout << "\n";
        }
        std::cout << "Enter message: ";
        char request[max_length];
        std::cin.getline(request, max_length);
        size_t request_length = strlen(request);
        boost::asio::write(_socket, boost::asio::buffer(request, request_length));

        char reply[max_length];
        size_t reply_length = boost::asio::read(_socket,
            boost::asio::buffer(reply, request_length));
        std::cout << "Reply is: ";
        std::cout.write(reply, reply_length);
        std::cout << "\n";
    }
    catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }
}
