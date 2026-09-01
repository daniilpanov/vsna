#pragma once
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

class NodeSession;

// Tracks the peers a node knows about. Distinguishes *known* peers (discovered
// through connections/acquaintances) from *connected* peers (those with a live
// session right now). Thread-safe: it is touched both from io threads (sessions)
// and from the UI.
class PeerRegistry {
  public:
	// Add an address to the set of known peers.
	void addKnown(const std::string& addr);

	// Mark a peer as connected and, implicitly, known.
	void addConnected(const std::string& addr, std::shared_ptr<NodeSession> session);

	// Drop a peer from the connected set (it stays known).
	void removeConnected(const std::string& addr);

	bool isKnown(const std::string& addr) const;
	bool isConnected(const std::string& addr) const;

	std::vector<std::string> known() const;
	std::vector<std::string> connected() const;
	std::size_t knownCount() const;
	std::size_t connectedCount() const;

  private:
	mutable std::mutex _mutex;
	std::set<std::string> _known;
	std::unordered_map<std::string, std::shared_ptr<NodeSession>> _connected;
};
