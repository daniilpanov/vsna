#include "server.h"

void Server::start() {
    // Parse the config
    Addr const addr = _config.getAddr();
    auto const ip = boost::asio::ip::make_address(addr.ip());
    auto const port = addr.portNum();

    // Create input/output acceptor
    boost::asio::io_context ioc { 1 };
    tcp::acceptor acceptor { ioc, {ip, port} };

    std::cout << "Addr: " << addr.toString() << std::endl;
    std::cout << "Listening for the connect...\n";
    while (1) {
        tcp::socket socket { ioc };
        acceptor.accept(socket);

        // Create a new thread with connect
        std::thread{[q = std::move(socket)]() mutable {
            // Accept the connect request
            boost::beast::websocket::stream<tcp::socket> ws {std::move(q)};
            ws.accept();
            std::cout << "Accepted a connect!\n";
            
            // Read the message
            while(1) {
                boost::beast::flat_buffer buffer;
                ws.read(buffer);
                auto out = boost::beast::buffers_to_string(buffer.cdata());
                std::cout << out << std::endl;
            }
        }}.detach();
    }
}