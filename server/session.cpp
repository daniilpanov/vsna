#include "session.h"


void ServerSession::run() {
    // Set suggested timeout settings for the websocket
    _ws.set_option(
        websocket::stream_base::timeout::suggested(
            beast::role_type::server
        )
    );

    // Set a decorator to change the Server of the handshake
    _ws.set_option(
        websocket::stream_base::decorator(
            [](websocket::response_type& res) {
                res.set(boost::beast::http::field::server,
                    std::string(BOOST_BEAST_VERSION_STRING) +
                        " websocket-server-async");
        }));

    // Accept the websocket handshake
    _ws.async_accept(
        beast::bind_front_handler(
            &ServerSession::on_accept, shared_from_this()
        )
    );
}


void ServerSession::on_accept(beast::error_code ec) {
    if (ec)
        return fail(ec, "accept");

    // Read the message
    do_read();
}


void ServerSession::do_read() {
    // Read a message into the buffer
    _ws.async_read(
        _buffer,
        beast::bind_front_handler(
            &ServerSession::on_read,
            shared_from_this()
        )
    );
}


void ServerSession::on_read(beast::error_code ec, std::size_t bytes_transferred) {
    boost::ignore_unused(bytes_transferred);

    // This indicates that the session was closed
    if (ec == websocket::error::closed)
        return;

    if (ec)
        return fail(ec, "read");

    std::cout
        << "Received: "
        << beast::make_printable(_buffer.data())
        << '\n';

    // Echo the message back (TODO: Implement actual message handling logic)
    _ws.text(_ws.got_text());
    _ws.async_write(
        _buffer.data(),
        beast::bind_front_handler(
            &ServerSession::on_write,
            shared_from_this()
        )
    );
}


void ServerSession::on_write(beast::error_code ec, std::size_t bytes_transferred) {
    boost::ignore_unused(bytes_transferred);

    if (ec)
        return fail(ec, "write");

    // Clear the buffer
    _buffer.consume(_buffer.size());

    // Do another read
    do_read();
}
