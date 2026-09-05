#include "session.h"

#include <boost/beast/websocket.hpp>

#include "message.h"
#include "node.h"

NodeSession::NodeSession(Node& node, boost::asio::io_context& ioc)
    : _node(node), _ioc(ioc), _resolver(asio::make_strand(ioc)), _ws(asio::make_strand(ioc)),
      // All timers live on the session strand so their handlers are serialized
      // with the read/write handlers instead of racing them on the bare
      // io_context.
      _hello_timer(_ws.get_executor()), _hello_retry_timer(_ws.get_executor()),
      _ping_timer(_ws.get_executor()), _idle_timer(_ws.get_executor())
{}

void NodeSession::accept(tcp::socket socket)
{
	_remote = socket.remote_endpoint().address().to_string() + ":"
	          + std::to_string(socket.remote_endpoint().port());
	setup_hello();

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
	_remote = host + ":" + port;
	setup_hello();

	_resolver.async_resolve(
	    host, port, beast::bind_front_handler(&NodeSession::on_resolve, shared_from_this()));
}

// Called by the node during shutdown: deterministically close this session so
// its in-flight handlers observe the closed socket and finish.
void NodeSession::shutdown()
{
	close("node shutdown");
}

void NodeSession::send(const Message& msg)
{
	{
		std::lock_guard<std::mutex> lock(_write_mutex);
		if (_closing)
			return;
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
	send_hello();
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

	// Key the remote by its canonical ip:port — the same form peers announce in
	// peersList — so hostnames dialed by the user never leak into the registry.
	_remote = ep.address().to_string() + ":" + std::to_string(ep.port());

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
	send_hello();
}

void NodeSession::setup_hello()
{
	setup_hello_timer();
}

// Arm a deadline: the peer must deliver its first (reply) hello before the
// retry budget runs out or the connection is torn down. This keeps a half-open
// connection from hanging around forever when a peer dials but never exchanges
// a hello, while still allowing all HELLO_RETRIES retransmissions to land.
void NodeSession::setup_hello_timer()
{
	_hello_timer.expires_after(HELLO_RETRY * (HELLO_RETRIES + 1));
	_hello_timer.async_wait([self = shared_from_this()](beast::error_code ec) {
		if (ec)
			return;
		if (!self->_initialized)
			self->close("hello timeout");
	});
}

// Disconnect deterministically: stop all pending timers and close the socket.
// Closing the socket makes any in-flight read/write observe an error, ending the
// session's event loop.
void NodeSession::close(const char *reason)
{
	// TODO: make close() entry atomic — a strand handler and Node::stop() can run the body
	// concurrently; benign (idempotent), use a single compare_exchange on _closing.
	if (_closing)
		return;
	_closing = true;

	_hello_timer.cancel();
	_hello_retry_timer.cancel();
	_ping_timer.cancel();
	_idle_timer.cancel();

	// Drop the session from the node's registry so a closed session neither
	// leaks nor keeps the socket/buffers alive until the node is destroyed.
	_node.detach(this);

	std::cout << "[~] closing session to " << _remote << " (" << reason << ")\n";

	// Force the socket shut immediately. No timeout is armed: arming the
	// transport-level timer while both a read and a write are pending violates
	// Beast's preconditions (assertion failure), and a bare async_close would
	// hang against a peer that never completes the close handshake. Closing the
	// socket directly is deterministic — any in-flight read/write completion
	// observes it as an error and tears the session down.
	boost::system::error_code ignore;
	beast::get_lowest_layer(_ws).socket().shutdown(tcp::socket::shutdown_both, ignore);
	beast::get_lowest_layer(_ws).socket().close(ignore);
}

// A frame is a valid hello only if it is typed hello. The hello is purely the
// initialization signal, so any non-hello frame is rejected during the handshake.
bool NodeSession::isValidHello(const Message& msg)
{
	return msg.type == MessageType::Hello;
}

void NodeSession::send_hello()
{
	if (_closing || _initialized || _hello_sends > HELLO_RETRIES)
		return;

	Message hello;
	hello.type = MessageType::Hello;
	hello.payload = json::object();
	send(hello);
	++_hello_sends;

	if (_hello_sends > HELLO_RETRIES)
		return;

	// Reschedule: keep retransmitting the greeting every HELLO_RETRY until the
	// peer's first reply hello lands (which cancels this timer) or the retry
	// budget is exhausted.
	_hello_retry_timer.expires_after(HELLO_RETRY);
	_hello_retry_timer.async_wait(
	    [self = shared_from_this()](beast::error_code ec) { self->retry_hello(ec); });
}

// One tick of the outgoing hello retry. Stop entirely once the peer's hello
// arrived (handled in on_hello/on_read) or the retry budget is used up.
void NodeSession::retry_hello(beast::error_code ec)
{
	if (ec || _closing || _initialized)
		return;
	send_hello();
}

void NodeSession::on_hello(const Message& msg)
{
	boost::ignore_unused(msg);

	// The remote peer's first hello is the reply to ours: stop retransmitting.
	_hello_retry_timer.cancel();
}

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
	if (ec || _closing)
		return;
	send_ping();
}

void NodeSession::send_ping()
{
	if (_closing)
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
// fresh IDLE_TIMEOUT window. Called on every successfully parsed incoming frame,
// above message dispatch, so control frames keep the connection alive too.
void NodeSession::refresh_idle()
{
	_idle_timer.expires_after(IDLE_TIMEOUT);
	_idle_timer.async_wait([self = shared_from_this()](beast::error_code ec) {
		if (ec)
			return;
		self->close("idle timeout");
	});
}

void NodeSession::do_read()
{
	_ws.async_read(_buffer, beast::bind_front_handler(&NodeSession::on_read, shared_from_this()));
}

void NodeSession::on_read(beast::error_code ec, std::size_t bytes_transferred)
{
	boost::ignore_unused(bytes_transferred);

	if (ec && ec != websocket::error::closed)
	{
		close(ec.message().c_str());
		return;
	}

	if (ec == websocket::error::closed)
	{
		close("peer closed");
		return;
	}

	// Deserialize the JSON frame into a message envelope and route it.
	// Parse directly from the flat_buffer memory via string_view to avoid the
	// intermediate std::string that buffers_to_string would create.
	auto data = _buffer.data();
	json j = json::parse(std::string_view(static_cast<const char *>(data.data()), data.size()),
	                     nullptr, false);
	_buffer.consume(_buffer.size());

	if (!_initialized)
	{
		// The handshake is strict: the first frame must be a valid hello (the
		// initialization signal) and arrive before the retry budget runs out
		// (setup_hello_timer). A ping may arrive before the handshake completes
		// (the peer can initialize sooner) and is safely ignored. Any other type
		// — or garbage is a protocol violation and ends the
		// connection.
		if (j.is_discarded())
		{
			close("non-JSON hello");
			return;
		}

		Message msg;
		try
		{
			msg = Message::fromJson(j);
		}
		catch (const std::exception&)
		{
			close("malformed hello");
			return;
		}

		if (msg.type == MessageType::Ping)
		{
			refresh_idle();
			do_read();
			return;
		}

		if (!isValidHello(msg))
		{
			close("expected hello");
			return;
		}
		refresh_idle();
		_initialized = true;
		_hello_timer.cancel();
		on_hello(msg);
		do_read();
		return;
	}

	// Once initialized the connection is lenient about transport junk: a
	// malformed or non-JSON frame is logged and skipped rather than killing the
	// live session.
	if (j.is_discarded())
	{
		std::cerr << "[!] Received a non-JSON frame, ignoring\n";
		do_read();
		return;
	}

	Message msg;
	try
	{
		msg = Message::fromJson(j);
	}
	catch (const std::exception&)
	{
		std::cerr << "[!] Received a malformed frame, ignoring\n";
		do_read();
		return;
	}

	// Any successfully parsed incoming frame (control or otherwise) resets the
	// idle deadline. This runs above dispatch so ping frames keep us alive too.
	refresh_idle();

	// A ping carries no meaning to us; it only refreshes the idle deadline above.
	if (msg.type == MessageType::Ping)
	{
		do_read();
		return;
	}

	// A hello after initialization does nothing (there is no "else"): the
	// connection is already initialized, so a stale retransmission is skipped.
	if (msg.type == MessageType::Hello)
	{
		std::cerr << "[!] Ignoring stale hello after handshake\n";
		do_read();
		return;
	}

	route(msg);

	// Keep the connection alive and read the next frame.
	do_read();
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
		// The connection is dead: close it instead of leaving the write queue
		// stuck and the read loop running on a broken socket.
		close(ec.message().c_str());
		return;
	}

	{
		std::lock_guard<std::mutex> lock(_write_mutex);
		_write_queue.pop_front();
	}

	do_write_next();
}
