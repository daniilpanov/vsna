#pragma once
#include <cstdint>
#include <fstream>
#include <memory>
#include <string>
#include <unordered_map>

#include "message.h"
#include "pch.h"

class Node;
class NodeSession;

// Coordinates a transactional file transfer over a single session. A transfer
// is a claim/commit transaction: the sender claims a transaction id, transfers
// the file bytes, then commits. The receiver never writes to the final path
// until commit; it stages bytes in a temp file that is atomically renamed into
// place on commit (or deleted on abort/error). This guarantees the destination
// never holds a partial file -- exactly the property the network is missing
// today.
//
// All transaction state is tied to one NodeSession and the two endpoints run
// it as a state machine around the message types claim / claim_ack / data /
// data_ack / commit / commit_ack / abort, multiplexed by tx_id.
class TransactionManager : public std::enable_shared_from_this<TransactionManager> {
  public:
	TransactionManager(Node& node, std::shared_ptr<NodeSession> session);

	// Sender side: send a local file to the peer over this session. Safe to
	// call from any thread; the transaction state machine runs on the
	// session's strand.
	void sendFile(const std::string& localPath);

	// Receiver side: entry point for a claim received on the session's default
	// handler. Runs on the io strand.
	void handleClaim(const Message& msg);

  private:
	struct Incoming
	{
		std::ofstream stream;
		std::string stagingPath;
		std::string finalPath;
	};

	struct Outgoing
	{
		uint64_t txId;
		std::string localPath;
		int phase; // 0 = claimed, 1 = data sent, 2 = done
	};

	Node& _node;
	std::shared_ptr<NodeSession> _session;
	std::unordered_map<uint64_t, Incoming> _incoming;
	std::unordered_map<uint64_t, Outgoing> _outgoing;
	uint64_t _nextId{ 1 };

	// Runs on the session strand.
	void do_sendFile(const std::string& localPath);
	void do_claim(uint64_t txId, const std::string& localPath);
	void handleReply(uint64_t txId, const Message& msg);
	void handleData(uint64_t txId, const Message& msg);
	void finishIncoming(uint64_t txId, bool commit);
	void abort(uint64_t txId, const std::string& reason);

	// Received-file bookkeeping helpers (filesystem access, called on strand).
	bool stageClaim(uint64_t txId, const Message& msg);
	void reply(uint64_t txId, MessageType type, json payload);
	void removeOutgoing(uint64_t txId);
};
