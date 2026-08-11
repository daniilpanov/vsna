#include "session.h"

// TODO: It can also be adapted to the Client
void session(socket_ptr sock) {
    try {
        for (;;) {
            char data[max_length];
            boost::system::error_code error;
            size_t length = sock->read_some(boost::asio::buffer(data), error);

            if (error == boost::asio::error::eof) {
                break;
            } else if (error) {
                throw boost::system::system_error(error); // Some other error.
            }

            std::cout << "Received: " << data << std::endl;
            boost::asio::write(*sock, boost::asio::buffer(data, length));
        }
    } catch (std::exception& e) {
        std::cerr << "Exception in thread: " << e.what() << "\n";
    }
}