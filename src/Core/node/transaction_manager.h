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

// Coordinates a transactional, chunked file transfer over a single session. A
// transfer is a claim/commit transaction: the sender claims a transaction id,
// streams the file as a sequence of data chunks (each acknowledged by the
// receiver before the next is sent), then commits.
//
// Two safety properties hold:
//  - No partial file ever appears at the final path: the receiver stages all
//    bytes in a hidden temp file and atomically renames it into place only on
//    commit (or deletes it on abort/error).
//  - Transfers can be resumed: a staged temp file left behind by an interrupted
//    transfer is reused. The sender queries the receiver with a status request
//    to learn how much has already been received and continues from there.
//
// All transaction state is tied to one NodeSession and the two endpoints run it
// as a state machine around the message types claim / claim_ack / data /
// data_ack / commit / commit_ack / abort / status / status_ack, multiplexed by
// tx_id.
class TransactionManager : public std::enable_shared_from_this<TransactionManager> {
  public:
	TransactionManager(Node& node, std::shared_ptr<NodeSession> session);

	// Sender side: send a local file to the peer over this session, resuming
	// any partial already staged on the peer. Safe to call from any thread; the
	// transaction state machine runs on the session's strand.
	void sendFile(const std::string& localPath);

	// Receiver side: entry point for claim / status frames received on the
	// session's default handler. Runs on the io strand.
	void handleDefault(const Message& msg);

  private:
	struct Incoming
	{
		std::ofstream stream;
		std::string stagingPath;
		std::string finalPath;
		uint64_t size;
	};

	struct Outgoing
	{
		uint64_t txId;
		std::string localPath;
		uint64_t totalSize;
		uint64_t nextOffset;
	};

	Node& _node;
	std::shared_ptr<NodeSession> _session;
	std::unordered_map<uint64_t, Incoming> _incoming;
	std::unordered_map<uint64_t, Outgoing> _outgoing;
	uint64_t _nextId{ 1 };

	// ---- sender state machine (runs on the session strand) ----
	void do_sendFile(const std::string& localPath);
	void sendChunk(Outgoing& tx);
	void handleReply(uint64_t txId, const Message& msg);

	// ---- receiver state machine (runs on the session strand) ----
	void onClaim(uint64_t txId, const Message& msg);
	void onStatus(const Message& msg);
	void handleData(uint64_t txId, const Message& msg);
	void finishIncoming(uint64_t txId, bool commit);

	// bookkeeping helpers (filesystem access, called on strand)
	uint64_t resumeOffset(const std::string& name, uint64_t size) const;
	void reply(uint64_t txId, MessageType type, json payload);
	void removeOutgoing(uint64_t txId);
	void abort(uint64_t txId, const std::string& reason);
};
