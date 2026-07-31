#include "server.h"

void start_server(const Config& config) {
    Addr const addr = config.addr;
    auto const ip = boost::asio::ip::make_address(addr.ip());
    auto const port = addr.port();

    boost::asio::io_context ioc { 1 };
    tcp::acceptor acceptor { ioc, {ip, port} };

    std::cout << "Listening for the connect...\n";
    while (1) {
        tcp::socket socket { ioc };
        acceptor.accept(socket);

        std::thread{[q = std::move(socket)]() mutable {

            boost::beast::websocket::stream<tcp::socket> ws {std::move(q)};
            ws.accept();

            while(1) {
                boost::beast::flat_buffer buffer;
                ws.read(buffer);
                auto out = boost::beast::buffers_to_string(buffer.cdata());
                std::cout << out << std::endl;
            }

        }}.detach();
    }
}