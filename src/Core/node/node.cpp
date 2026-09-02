#include "node.h"

#include <unistd.h>

#include "transaction_manager.h"

Node::Node() : _io_context(), _acceptor(_io_context)
{}

Node::~Node()
{
	stop();
}

void Node::setup_acceptor()
{
	beast::error_code ec;
	tcp::endpoint endpoint(asio::ip::make_address(_config.getAddr().ip()),
	                       _config.getAddr().portNum());

	_acceptor.open(endpoint.protocol(), ec);
	if (ec)
		return fail(ec, "open");

	_acceptor.set_option(asio::socket_base::reuse_address(true), ec);
	if (ec)
		return fail(ec, "set_option");

	_acceptor.bind(endpoint, ec);
	if (ec)
		return fail(ec, "bind");

	_acceptor.listen(asio::socket_base::max_listen_connections, ec);
	if (ec)
		return fail(ec, "listen");

	std::cout << "[~] Listening on " << _config.getAddr().toString() << '\n';
}

void Node::run()
{
	setup_acceptor();
	do_accept();

	_threads.reserve(max_threads - 1);
	for (size_t i = 0; i < max_threads - 1; ++i)
	{
		_threads.emplace_back([this] { _io_context.run(); });
	}

	std::cout << "[~] Node started (PID " << getpid() << ")\n";
	_io_context.run();
}

void Node::start()
{
	setup_acceptor();
	do_accept();

	_threads.reserve(max_threads - 1);
	for (size_t i = 0; i < max_threads - 1; ++i)
	{
		_threads.emplace_back([this] { _io_context.run(); });
	}

	std::cout << "[~] Node started (PID " << getpid() << ")\n";
}

void Node::connect(const std::string& host, const std::string& port)
{
	auto session = std::make_shared<NodeSession>(*this, _io_context);
	{
		std::lock_guard<std::mutex> lock(_sessions_mutex);
		_sessions.push_back(session);
	}
	session->dial(host, port);
}

void Node::addPeer(const std::string& host, const std::string& port)
{
	_peers.addKnown(host + ":" + port);
}

void Node::connectToAllKnown()
{
	for (const auto& addr : _peers.known())
	{
		if (_peers.isConnected(addr))
			continue;
		auto parts = split(addr, ":");
		if (parts.size() == 2L)
			connect(parts[0], parts[1]);
	}
}

void Node::sendFile(const std::string& peerAddr, const std::string& localPath)
{
	std::shared_ptr<NodeSession> session = _peers.sessionFor(peerAddr);
	if (!session)
		session = _peers.firstSession();
	if (!session)
	{
		std::cerr << "[!] No connected peer to send to\n";
		return;
	}
	session->txn()->sendFile(localPath);
}

void Node::stop()
{
	_io_context.stop();
	for (auto& t : _threads)
	{
		if (t.joinable())
			t.join();
	}
	_threads.clear();
}

void Node::do_accept()
{
	_acceptor.async_accept(asio::make_strand(_io_context),
	                       beast::bind_front_handler(&Node::on_accept, shared_from_this()));
}

void Node::on_accept(beast::error_code ec, tcp::socket socket)
{
	if (ec)
	{
		if (ec == asio::error::operation_aborted)
			return;
		fail(ec, "accept");
	}
	else
	{
		std::cout << "[~] Accepted " << socket.remote_endpoint().address().to_string() << ':'
		          << socket.remote_endpoint().port() << '\n';

		auto session = std::make_shared<NodeSession>(*this, _io_context);
		{
			std::lock_guard<std::mutex> lock(_sessions_mutex);
			_sessions.push_back(session);
		}
		session->accept(std::move(socket));
	}

	do_accept();
}
