#include "node.h"

#include <algorithm>
#include <unistd.h>

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

void Node::print() const
{
	std::cout << _config.toString() << std::endl;
}

void Node::myPath() const
{
	std::cout << "[=] Node path: " << _config.getPath() << std::endl;
}

void Node::stop()
{
	// Stop accepting new connections so no further sessions can be created.
	beast::error_code ignore;
	_acceptor.close(ignore);

	_io_context.stop();
	for (auto& t : _threads)
	{
		if (t.joinable())
			t.join();
	}
	_threads.clear();

	// stop() drops queued handlers instead of running them, yet every pending
	// read/write/timer still holds a shared_ptr to its session. If those
	// handlers were left queued they would keep the sessions alive until the
	// io_context is destroyed in the destructor — after the sessions themselves
	// are gone. Drain them explicitly: each session is shut down first, so the
	// remaining handlers complete as errors and release their references.
	_io_context.restart();
	for (;;)
	{
		for (const auto& session : sessionsSnapshot())
			session->shutdown();
		if (_io_context.poll() == 0)
			break;
	}
}

std::vector<std::shared_ptr<NodeSession>> Node::sessionsSnapshot()
{
	std::lock_guard<std::mutex> lock(_sessions_mutex);
	return _sessions;
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

void Node::detach(NodeSession *session)
{
	std::lock_guard<std::mutex> lock(_sessions_mutex);
	_sessions.erase(std::remove_if(_sessions.begin(), _sessions.end(),
	                               [session](const auto& s) { return s.get() == session; }),
	                _sessions.end());
}
