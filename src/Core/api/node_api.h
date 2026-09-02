#pragma once
#include <functional>
#include <memory>
#include <string>
#include <vector>

class Node;

// Thin API facade the UI talks to. It owns a Node and exposes high-level
// operations using only plain types, so the presentation layer never touches
// the networking / transaction Core directly. This is the UI -> API -> Core
// seam the architecture calls for.
class NodeApi {
  public:
	NodeApi();
	~NodeApi();

	// Configure the underlying node before it is started.
	void configure(const std::string& ip, const std::string& port, const std::string& path);
	void configureFromFile(const std::string& configFile);

	// Human-readable description of the active configuration.
	std::string describe() const;
	std::string addr() const;
	std::string path() const;

	void start();
	void stop();

	// Peer management.
	void connect(const std::string& host, const std::string& port);
	void addPeer(const std::string& host, const std::string& port);
	void connectToAllKnown();
	std::vector<std::string> knownPeers() const;
	std::vector<std::string> connectedPeers() const;

	// Start a transactional transfer to a connected peer.
	void sendFile(const std::string& peerAddr, const std::string& localPath);

	// Notify the caller whenever a brand-new peer becomes known.
	void setOnPeerDiscovered(std::function<void(const std::string&)> obs);

  private:
	std::shared_ptr<Node> _node;
};