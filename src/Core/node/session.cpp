#include "session.h"

#include <boost/beast/websocket.hpp>

#include "message.h"

NodeSession::NodeSession(Node& node, boost::asio::io_context& ioc)
    : _node(node), _ioc(ioc), _resolver(asio::make_strand(ioc)), _ws(asio::make_strand(ioc)),
      // All timers live on the session strand so their handlers are serialized
      // with the read/write handlers instead of racing them on the bare
      // io_context.
      _ping_timer(_ws.get_executor()), _idle_timer(_ws.get_executor())
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
		// A non-ping frame written counts as keepalive activity, so the next
		// scheduled ping is skipped.
		if (msg.type != MessageType::Ping)
			_ping_suppressed = true;
		if (_writing)
			return;
		_writing = true;
	}

	asio::dispatch(_ws.get_executor(), [self = shared_from_this()] { self->do_write_next(); });
}

void NodeSession::onType(MessageType type, Handler handler)
{
	_type_handlers[type] = std::move(handler);
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

	setup_keepalive();
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

	setup_keepalive();
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
	{
		_ping_timer.cancel();
		_idle_timer.cancel();
		return;
	}

	if (ec)
	{
		_ping_timer.cancel();
		_idle_timer.cancel();
		return fail(ec, "read");
	}

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
		Message msg = Message::fromJson(j);

		// Any successfully parsed incoming frame (control or otherwise) resets the
		// idle deadline. This runs above dispatch so ping frames keep us alive too.
		refresh_idle();

		// A ping carries no meaning to us; it only refreshes the idle deadline above.
		if (msg.type != MessageType::Ping)
			route(msg);
	}

	// Keep the connection alive and read the next frame.
	do_read();
}

// Arm the keepalive: refresh the idle deadline and schedule the first ping.
void NodeSession::setup_keepalive()
{
	refresh_idle();
	schedule_ping();
}

// Arm the keepalive ping: every PING_INTERVAL a ping is sent, unless a non-ping
// frame was already written since the previous tick (that counts as activity).
void NodeSession::schedule_ping()
{
	_ping_timer.expires_after(PING_INTERVAL);
	_ping_timer.async_wait(
	    [self = shared_from_this()](beast::error_code ec) { self->retry_ping(ec); });
}

void NodeSession::retry_ping(beast::error_code ec)
{
	if (ec || _closed)
		return;
	send_ping();
}

void NodeSession::send_ping()
{
	if (_closed)
		return;

	// A non-ping frame written since the last tick already kept the connection
	// alive; skip this ping and re-arm the timer. Send() may set the flag from
	// another thread, so read/reset it under the write mutex.
	bool suppressed;
	{
		std::lock_guard<std::mutex> lock(_write_mutex);
		suppressed = _ping_suppressed;
		_ping_suppressed = false;
	}
	if (suppressed)
	{
		schedule_ping();
		return;
	}

	Message ping;
	ping.type = MessageType::Ping;
	ping.payload = json::object();
	send(ping);

	schedule_ping();
}

// Reset the disconnection deadline: any incoming frame (including a ping) arms a
// fresh IDLE_TIMEOUT window. On expiry the connection is torn down.
void NodeSession::refresh_idle()
{
	_idle_timer.expires_after(IDLE_TIMEOUT);
	_idle_timer.async_wait([self = shared_from_this()](beast::error_code ec) {
		if (ec)
			return;
		self->tear_down();
	});
}

// Close the connection. Closing the lowest layer makes the in-flight read (and
// any queued write) fail, which ends the session's event loop.
void NodeSession::tear_down()
{
	if (_closed)
		return;
	_closed = true;
	_ping_timer.cancel();
	_idle_timer.cancel();
	beast::get_lowest_layer(_ws).close();
}

void NodeSession::route(const Message& msg)
{
	auto it = _type_handlers.find(msg.type);
	if (it != _type_handlers.end())
	{
		it->second(msg);
		return;
	}
	if (_default_handler)
	{
		_default_handler(msg);
		return;
	}
	std::cout << "[~] Unhandled message (type=" << json(msg.type).get<std::string>() << ")\n";
}

void NodeSession::on_write(beast::error_code ec, std::size_t bytes_transferred)
{
	boost::ignore_unused(bytes_transferred);

	if (ec)
	{
		_ping_timer.cancel();
		_idle_timer.cancel();
		fail(ec, "write");
		return;
	}

	{
		std::lock_guard<std::mutex> lock(_write_mutex);
		_write_queue.pop_front();
	}

	do_write_next();
}
