#pragma once
#include <memory>
#include <thread>
#include <vector>

#include "config.h"
#include "helper.h"
#include "session.h"

// A symmetric peer node. Unlike the old split server/client, a single Node both
// listens for incoming connections and dials out to other nodes. Only the
// direction of the first connection (who dialed first) differs between the two
// sides; both speak the same protocol.
class Node : public std::enable_shared_from_this<Node> {
  public:
	Node();
	~Node();

	void setConfig(const Config& config)
	{
		_config = config;
	}
	Config getConfig() const
	{
		return _config;
	}

	// Start listening on the configured address and block the calling thread.
	void run();

	// Start listening and spawn worker threads, then return immediately.
	void start();

	// Dial a remote peer at host:port.
	void connect(const std::string& host, const std::string& port);

	// Stop the io_context and all worker threads.
	void stop();

	// Print the node configuration (UI helper).
	void print() const;

	// Print the node's local share path (UI helper).
	void myPath() const;

  private:
	Config _config;
	boost::asio::io_context _io_context;
	tcp::acceptor _acceptor;
	std::vector<std::thread> _threads;
	std::vector<std::shared_ptr<NodeSession>> _sessions;

	void setup_acceptor();
	void do_accept();
	void on_accept(beast::error_code ec, tcp::socket socket);
};
