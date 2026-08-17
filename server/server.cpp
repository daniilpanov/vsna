#include "server.h"

Server::Server(): 
        _config(Config::loadFromFile("config.json")), // TODO: Fix hardcode
        _io_context(max_threads),
        _acceptor(_io_context)
    {
        beast::error_code ec;
        tcp::endpoint endpoint(
            asio::ip::make_address(_config.getAddr().ip()),
            _config.getAddr().portNum()
        );

        // Open the acceptor
        _acceptor.open(endpoint.protocol(), ec);
        if (ec) {
            fail(ec, "open");
            return;
        }
        std::cout << "opened\n";

        // Allow address reuse
        _acceptor.set_option(
            asio::socket_base::reuse_address(true), ec
        );
        if (ec) {
            fail(ec, "set_oprion");
            return;
        }
        std::cout << "set_option\n";

        // Bind to the server address
        _acceptor.bind(endpoint, ec);
        if (ec) {
            fail(ec, "bind");
            return;
        }
        std::cout << "binded\n";

        // Start listening for connections
        _acceptor.listen(
            asio::socket_base::max_listen_connections, ec
        );
        if (ec) {
            fail(ec, "listen");
            return;
        }
        std::cout << "listening\n";
    }


void Server::run() {
    start_accept();
    std::cout << "Server is running on " << _config.getAddr().toString() << std::endl;

    _threads.reserve(max_threads - 1);

    for (size_t i = 0; i < max_threads - 1; ++i) {
        _threads.emplace_back(
            [this] {
                _io_context.run();
            }
        );
        std::cout << "Thread " << i + 1 << " started." << std::endl;
    }

    std::cout << "PID: " << getpid() << '\n';
    std::cout << "Main thread started." << std::endl;
    _io_context.run();
}


void Server::start_accept() {
    do_accept();
}


void Server::do_accept() {
    std::cout << "Waiting for a TCP connection...\n";

    // The new connection gets its own strand
    _acceptor.async_accept(
        asio::make_strand(_io_context),
        beast::bind_front_handler(
            &Server::on_accept, shared_from_this()
        )
    );
}


void Server::on_accept(beast::error_code ec, tcp::socket socket) {
    if (ec) {
        fail(ec, "accept");
    }
    else {
        std::cout << "Accepted "
                  << socket.remote_endpoint().address().to_string()
                  << ':'
                  << socket.remote_endpoint().port()
                  << '\n';
                  
        // Create the session and run it
        std::make_shared<ServerSession>(std::move(socket))->run();
    }

    // Accept another connection
    do_accept();
}