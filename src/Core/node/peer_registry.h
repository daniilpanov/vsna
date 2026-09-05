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

	// Mark a peer as connected based on a remote update, without owning a local
	// session for it. Used when merging a peersList frame announcing connected
	// peers that this node did not itself dial or accept.
	void markConnected(const std::string& addr);

	// Drop a peer from the connected set (it stays known). Used for remote
	// updates where the peer is reported idle.
	void removeConnected(const std::string& addr);

	// Drop a single live connection to a peer. Other connections to the same
	// address keep it marked connected.
	void disconnect(const std::string& addr, NodeSession *session);

	bool isKnown(const std::string& addr) const;
	bool isConnected(const std::string& addr) const;

	std::vector<std::string> known() const;
	std::vector<std::string> connected() const;
	std::size_t knownCount() const;
	std::size_t connectedCount() const;

  private:
	mutable std::mutex _mutex;
	std::set<std::string> _known;
	// One slot per live connection to an address. A nullptr slot marks a peer
	// reported connected by a remote node (we know it is connected in the mesh
	// but hold no session for it). An address stops being "connected" only when
	// its last slot goes away.
	std::unordered_map<std::string, std::vector<std::shared_ptr<NodeSession>>> _connected;
};
