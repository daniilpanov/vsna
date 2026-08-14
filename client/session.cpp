#include "session.h"


void ClientSession::run(char const* host, char const* port, char const* text) {
    _host = host;
    _port = port;
    _text = text;

    _resolver.async_resolve(
        host, port,
        beast::bind_front_handler(
            &ClientSession::on_resolve, shared_from_this()
        )
    );
}


void ClientSession::on_resolve(
    beast::error_code ec,
    tcp::resolver::results_type results
) {
    if (ec)
        return fail(ec, "resolve");

    // Set the timeout for the operation
    beast::get_lowest_layer(_ws).expires_after(std::chrono::seconds(30));

    // Make the connection on the IP address we get from a lookup
    beast::get_lowest_layer(_ws).async_connect(
        results,
        beast::bind_front_handler(
            &ClientSession::on_connect, shared_from_this()
        )
    );
}


void ClientSession::on_connect(beast::error_code ec, tcp::resolver::results_type::endpoint_type ep) {
    if(ec)
        return fail(ec, "connect");

    // Turn off the timeout on the tcp_stream, because
    // the websocket stream has its own timeout system.
    beast::get_lowest_layer(_ws).expires_never();

    // Set suggested timeout settings for the websocket
    _ws.set_option(
        websocket::stream_base::timeout::suggested(
            beast::role_type::client
        )
    );

    // Set a decorator to change the User-Agent of the handshake
    _ws.set_option(websocket::stream_base::decorator(
        [](websocket::request_type& req)
        {
            req.set(beast::http::field::user_agent,
                std::string(BOOST_BEAST_VERSION_STRING) +
                    " websocket-client-async");
        }));

    // Perform the websocket handshake
    _ws.async_handshake(
        _host, "/",
        beast::bind_front_handler(
            &ClientSession::on_handshake, shared_from_this()
        )
    );
}


void ClientSession::on_handshake(beast::error_code ec) {
    if (ec)
        return fail(ec, "handshake");

    _ws.async_write(
        asio::buffer(_text),
        beast::bind_front_handler(
            &ClientSession::on_write, shared_from_this()
        )
    );
}


void ClientSession::on_write(
    beast::error_code ec,
    std::size_t bytes_transferred
) {
    if (ec)
        return fail(ec, "write");

    boost::ignore_unused(bytes_transferred);

    // Read a message into the buffer
    _ws.async_read(
        _buffer,
        beast::bind_front_handler(
            &ClientSession::on_read, shared_from_this()
        )
    );
}


void ClientSession::on_read(
    beast::error_code ec,
    std::size_t bytes_transferred
) {
    if (ec)
        return fail(ec, "read");

    boost::ignore_unused(bytes_transferred);

    // Process the response from the server

    // Close the WebSocket connection (TODO: Do not close the websocket after one message, keep it living)
    _ws.async_close(websocket::close_code::normal,
        beast::bind_front_handler(
            &ClientSession::on_close, shared_from_this()
        )
    );
}


void ClientSession::on_close(beast::error_code ec) {
    if (ec) 
        return fail(ec, "close");

    // TODO: Add a normal close
    std::cout << beast::make_printable(_buffer.data()) << std::endl;
}