#pragma once
#include <functional>
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
	using Observer = std::function<void(const std::string&)>;

	// Add an address to the set of known peers.
	void addKnown(const std::string& addr);

	// Mark a peer as connected and, implicitly, known.
	void addConnected(const std::string& addr, std::shared_ptr<NodeSession> session);

	// Drop a peer from the connected set (it stays known).
	void removeConnected(const std::string& addr);

	// Called (outside the internal lock) whenever a brand-new peer becomes known.
	void setOnDiscovered(Observer obs);

	bool isKnown(const std::string& addr) const;
	bool isConnected(const std::string& addr) const;

	// Look up the live session for a connected peer address (may be null).
	std::shared_ptr<NodeSession> sessionFor(const std::string& addr) const;
	std::shared_ptr<NodeSession> firstSession() const;

	std::vector<std::string> known() const;
	std::vector<std::string> connected() const;
	std::size_t knownCount() const;
	std::size_t connectedCount() const;

  private:
	mutable std::mutex _mutex;
	std::set<std::string> _known;
	std::unordered_map<std::string, std::shared_ptr<NodeSession>> _connected;
	Observer _on_discovered;
};
