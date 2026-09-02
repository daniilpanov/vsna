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

	// Registry of known / connected peers.
	PeerRegistry& peers()
	{
		return _peers;
	}

	// Notify the caller whenever a brand-new peer becomes known.
	void setOnPeerDiscovered(PeerRegistry::Observer obs)
	{
		_peers.setOnDiscovered(std::move(obs));
	}

	// Manually add a peer to the known set without dialing it.
	void addPeer(const std::string& host, const std::string& port);

	// Dial every known peer that is not already connected.
	void connectToAllKnown();

	// Start a transactional transfer of a local file to a connected peer. If
	// peerAddr is empty or not a live connection, the first connected peer is
	// used. The file arrives either fully committed or is rolled back.
	void sendFile(const std::string& peerAddr, const std::string& localPath);

  private:
	Config _config;
	PeerRegistry _peers;
	boost::asio::io_context _io_context;
	tcp::acceptor _acceptor;
	std::vector<std::thread> _threads;
	std::mutex _sessions_mutex;
	std::vector<std::shared_ptr<NodeSession>> _sessions;

	void setup_acceptor();
	void do_accept();
	void on_accept(beast::error_code ec, tcp::socket socket);
};
