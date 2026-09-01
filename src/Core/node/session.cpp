#include "session.h"

#include <boost/beast/websocket.hpp>

#include "message.h"
#include "node.h"
#include "transaction_manager.h"

NodeSession::NodeSession(Node& node, boost::asio::io_context& ioc)
    : _node(node), _ioc(ioc), _resolver(asio::make_strand(ioc)), _ws(asio::make_strand(ioc))
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
	send_hello();
}

void NodeSession::setup_hello()
{
	_handlers[HELLO_TX] = [this](const Message& msg) { on_hello(msg); };

	// Route transaction-claim/status frames to the transaction manager. Any
	// frame that reaches this default handler without its own transaction
	// handler is either a claim/status (transfer protocol) or unexpected.
	onMessage([this](const Message& msg) {
		if (msg.type == MessageType::Claim || msg.type == MessageType::Status)
		{
			txn()->handleDefault(msg);
			return;
		}
		std::cout << "[~] Unhandled message (type=" << json(msg.type).get<std::string>()
		          << ", tx_id=" << msg.tx_id << ")\n";
	});
}

void NodeSession::send_hello()
{
	Message hello;
	hello.type = MessageType::Hello;
	hello.tx_id = HELLO_TX;
	hello.payload = { { "me", _node.getConfig().getAddr().toString() },
		              { "known_peers", _node.peers().known() } };
	send(hello);
}

void NodeSession::on_hello(const Message& msg)
{
	// The remote peer both announces its listener address and lists the peers
	// it already knows (transitive discovery).
	const std::string me = msg.payload.value("me", _remote);
	_remote = me;
	_node.peers().addConnected(_remote, shared_from_this());

	if (!msg.payload.contains("known_peers") || !msg.payload["known_peers"].is_array())
		return;
	std::string self = _node.getConfig().getAddr().toString();
	for (const auto& p : msg.payload["known_peers"])
	{
		std::string addr = p.get<std::string>();
		if (addr != self)
			_node.peers().addKnown(addr);
	}
}

std::shared_ptr<TransactionManager> NodeSession::txn()
{
	if (!_txn)
		_txn = std::make_shared<TransactionManager>(_node, shared_from_this());
	return _txn;
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
		_node.peers().removeConnected(_remote);
		return;
	}

	if (ec)
	{
		_node.peers().removeConnected(_remote);
		return fail(ec, "read");
	}

	// Deserialize the JSON frame into a message envelope and route it.
	json j = json::parse(beast::buffers_to_string(_buffer.data()), nullptr, false);
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
