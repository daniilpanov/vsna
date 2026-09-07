#pragma once
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include "config.h"
#include "helper.h"
#include "peer_registry.h"
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
	void setConfig(Config&& config)
	{
		_config = std::move(config);
	}
	Config getConfig() const
	{
		return _config;
	}

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

	// Print the known and connected peers (UI helper).
	void printPeers() const;

	// Registry of known / connected peers.
	PeerRegistry& peers()
	{
		return _peers;
	}

  private:
	// NodeSession notifies the node when it closes (close -> detach).
	friend class NodeSession;

	Config _config;
	boost::asio::io_context _io_context;
	PeerRegistry _peers;
	tcp::acceptor _acceptor;
	std::vector<std::thread> _threads;
	// Sessions are pushed from the UI thread (connect) and from io threads
	// (on_accept) and popped when a session closes, so the vector is guarded.
	std::vector<std::shared_ptr<NodeSession>> _sessions;
	std::mutex _sessions_mutex;

	void setup_acceptor();
	void do_accept();
	void on_accept(beast::error_code ec, tcp::socket socket);

	// Copy the live sessions under the mutex.
	std::vector<std::shared_ptr<NodeSession>> sessionsSnapshot();

	// Drop a session from the registry once it has closed. Callers must hold a
	// shared reference to the session (for example a running handler), because
	// this releases the node's own reference and may be the last one.
	void detach(NodeSession *session);
};
