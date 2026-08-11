#include "session.h"


void session(socket_ptr sock) {
    char data[max_length];

    #ifdef BUILD_SERVER
    try {
        for (;;) {
            boost::system::error_code error;
            const size_t length = sock->read_some(boost::asio::buffer(data), error);

            if (error == boost::asio::error::eof) {
                break;
            } else if (error) {
                throw boost::system::system_error(error); // Some other error.
            }

            std::cout << "Received: ";
            std::cout.write(data, static_cast<std::streamsize>(length));
            std::cout << '\n';

            boost::asio::write(
                *sock,
                boost::asio::buffer(data, length)
            );
        }
    } catch (std::exception& e) {
        std::cerr << "Exception in thread: " << e.what() << "\n";
    }
    #endif

    #ifdef BUILD_CLIENT
    try {
        std::cout << "Enter the message";
        for (;;) {
            std::cout << ": ";

            if (!std::cin.getline(data, max_length)) {
                break;
            }

            const size_t length = std::strlen(data);
            if (length == 0) {
                continue;
            }

            boost::asio::write(
                *sock,
                boost::asio::buffer(data, length)
            );


            char reply[max_length];
            const size_t reply_length = boost::asio::read(
                *sock,
                boost::asio::buffer(reply, length)
            );

            std::cout << "Reply is: ";
            std::cout.write(
                reply,
                static_cast<std::streamsize>(reply_length)
            );
            std::cout << '\n';
        }
    }
    catch (const boost::system::system_error& e) {
        if (e.code() == boost::asio::error::eof) {
            std::cerr << "Server disconnected\n";
        } else {
            std::cerr << "Client session exception: "
                      << e.what() << '\n';
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Client session exception: "
                  << e.what() << '\n';
    }
    #endif
}