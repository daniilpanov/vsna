#include "client.h"

void Client::connect() {
    try {
        Addr addr = _config.getAddr();
        tcp::resolver::query query(tcp::v4(), addr.ip(), std::to_string(addr.port()));
        tcp::resolver::iterator it = _resolver.resolve(query);

        boost::asio::connect(_socket, it);

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