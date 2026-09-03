#include "session.h"

#include <boost/beast/websocket.hpp>

#include "message.h"

NodeSession::NodeSession(Node& node, boost::asio::io_context& ioc)
    : _node(node), _ioc(ioc), _resolver(asio::make_strand(ioc)), _ws(asio::make_strand(ioc))
{}

void NodeSession::accept(tcp::socket socket)
{
	// Wrap the accepted socket and open the websocket stream.
	_ws.next_layer().socket() = std::move(socket);

	// Set suggested timeout settings for the websocket.
	_ws.set_option(websocket::stream_base::timeout::suggested(beast::role_type::server));

	// Decorator to mark the server side of the handshake.
	_ws.set_option(websocket::stream_base::decorator([](websocket::response_type& res) {
		res.set(boost::beast::http::field::server,
		        std::string(BOOST_BEAST_VERSION_STRING) + " vsna-node");
	}));

	_ws.async_accept(beast::bind_front_handler(&NodeSession::on_accept, shared_from_this()));
}

void NodeSession::dial(const std::string& host, const std::string& port)
{
	_host = host;
	_port = port;

	_resolver.async_resolve(
	    host, port, beast::bind_front_handler(&NodeSession::on_resolve, shared_from_this()));
}

void NodeSession::send(const Message& msg)
{
	{
		std::lock_guard<std::mutex> lock(_write_mutex);
		_write_queue.push_back(msg);
		if (_writing)
			return;
		_writing = true;
	}

	asio::dispatch(_ws.get_executor(), [self = shared_from_this()] { self->do_write_next(); });
}

void NodeSession::onTx(uint64_t tx_id, Handler handler)
{
	_handlers[tx_id] = std::move(handler);
}

void NodeSession::onMessage(Handler handler)
{
	_default_handler = std::move(handler);
}

void NodeSession::do_write_next()
{
	{
		std::lock_guard<std::mutex> lock(_write_mutex);
		if (_write_queue.empty())
		{
			_writing = false;
			return;
		}
		_outgoing = _write_queue.front().toJson().dump();
	}

	_ws.text(true);
	_ws.async_write(asio::buffer(_outgoing),
	                beast::bind_front_handler(&NodeSession::on_write, shared_from_this()));
}

void NodeSession::on_accept(beast::error_code ec)
{
	if (ec)
		return fail(ec, "accept");

	do_read();
}

void NodeSession::on_resolve(beast::error_code ec, tcp::resolver::results_type results)
{
	if (ec)
		return fail(ec, "resolve");

	beast::get_lowest_layer(_ws).expires_after(std::chrono::seconds(30));

	beast::get_lowest_layer(_ws).async_connect(
	    results, beast::bind_front_handler(&NodeSession::on_connect, shared_from_this()));
}

void NodeSession::on_connect(beast::error_code ec, tcp::resolver::results_type::endpoint_type ep)
{
	if (ec)
		return fail(ec, "connect");

	// The websocket stream has its own timeout system.
	beast::get_lowest_layer(_ws).expires_never();

	_ws.set_option(websocket::stream_base::timeout::suggested(beast::role_type::client));

	_ws.set_option(websocket::stream_base::decorator([](websocket::request_type& req) {
		req.set(beast::http::field::user_agent,
		        std::string(BOOST_BEAST_VERSION_STRING) + " vsna-node");
	}));

	_ws.async_handshake(_host, "/",
	                    beast::bind_front_handler(&NodeSession::on_handshake, shared_from_this()));
}

void NodeSession::on_handshake(beast::error_code ec)
{
	if (ec)
		return fail(ec, "handshake");

	do_read();
}

void NodeSession::do_read()
{
	_ws.async_read(_buffer, beast::bind_front_handler(&NodeSession::on_read, shared_from_this()));
}

void NodeSession::on_read(beast::error_code ec, std::size_t bytes_transferred)
{
	boost::ignore_unused(bytes_transferred);

	if (ec == websocket::error::closed)
		return;

	if (ec)
		return fail(ec, "read");

	// Deserialize the JSON frame into a message envelope and route it.
	// Parse directly from the flat_buffer memory via string_view to avoid the
	// intermediate std::string that buffers_to_string would create.
	auto data = _buffer.data();
	json j = json::parse(std::string_view(static_cast<const char *>(data.data()), data.size()),
	                     nullptr, false);
	_buffer.consume(_buffer.size());
	if (j.is_discarded())
	{
		std::cerr << "[!] Received a non-JSON frame, ignoring\n";
	}
	else
	{
		route(Message::fromJson(j));
	}

	// Keep the connection alive and read the next frame.
	do_read();
}

void NodeSession::route(const Message& msg)
{
	auto it = _handlers.find(msg.tx_id);
	if (it != _handlers.end())
	{
		it->second(msg);
		return;
	}
	if (_default_handler)
	{
		_default_handler(msg);
		return;
	}
	std::cout << "[~] Unhandled message (type=" << json(msg.type).get<std::string>()
	          << ", tx_id=" << msg.tx_id << ")\n";
}

void NodeSession::on_write(beast::error_code ec, std::size_t bytes_transferred)
{
	boost::ignore_unused(bytes_transferred);

	if (ec)
	{
		fail(ec, "write");
		return;
	}

	{
		std::lock_guard<std::mutex> lock(_write_mutex);
		_write_queue.pop_front();
	}

	do_write_next();
}
