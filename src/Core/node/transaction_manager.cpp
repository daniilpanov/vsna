#include "transaction_manager.h"

#include <filesystem>
#include <fstream>

#include <boost/asio/post.hpp>

#include "base64.h"
#include "node.h"
#include "session.h"

namespace {
// Staging directory (relative to the node share path) for in-flight transfers.
constexpr const char *kStagingDir{ ".vsna-tmp" };
}

TransactionManager::TransactionManager(Node& node, std::shared_ptr<NodeSession> session)
    : _node(node), _session(std::move(session))
{}

// ---------------------------------------------------------------------------
// Sender
// ---------------------------------------------------------------------------

void TransactionManager::sendFile(const std::string& localPath)
{
	auto self = shared_from_this();
	asio::post(_session->strand(), [self, localPath] { self->do_sendFile(localPath); });
}

void TransactionManager::do_sendFile(const std::string& localPath)
{
	namespace fs = std::filesystem;

	std::error_code ec;
	uint64_t total = fs::file_size(localPath, ec);
	if (ec)
	{
		std::cerr << "[!] Cannot stat " << localPath << ": " << ec.message() << '\n';
		return;
	}

	uint64_t txId = _nextId++;
	_session->onTx(txId, [this, txId](const Message& msg) { handleReply(txId, msg); });
	_outgoing.emplace(txId, Outgoing{ txId, localPath, total, 0 });

	// Ask the peer how much of this file it already staged (resume support),
	// then claim and resume from that offset.
	Message status;
	status.type = MessageType::Status;
	status.tx_id = txId;
	status.payload = { { "name", fs::path(localPath).filename().string() }, { "size", total } };
	_session->send(status);
}

void TransactionManager::sendChunk(Outgoing& tx)
{
	if (tx.nextOffset >= tx.totalSize)
	{
		Message commit;
		commit.type = MessageType::Commit;
		commit.tx_id = tx.txId;
		_session->send(commit);
		return;
	}

	// Move a chunk of up to max_length bytes from the local file to the wire.
	// The next chunk (or the commit) is sent only after this one is acked.
	std::size_t len
	    = static_cast<std::size_t>(std::min<uint64_t>(max_length, tx.totalSize - tx.nextOffset));
	std::string buf(len, '\0');
	{
		std::ifstream in(tx.localPath, std::ios::binary);
		in.seekg(static_cast<std::streamoff>(tx.nextOffset));
		in.read(buf.data(), static_cast<std::streamsize>(len));
	}

	Message data;
	data.type = MessageType::Data;
	data.tx_id = tx.txId;
	data.payload = { { "offset", tx.nextOffset }, { "data", base64::encode(buf) } };
	_session->send(data);
}

void TransactionManager::handleReply(uint64_t txId, const Message& msg)
{
	auto it = _outgoing.find(txId);
	if (it == _outgoing.end())
		return;
	Outgoing& tx = it->second;

	switch (msg.type)
	{
	case MessageType::StatusAck: {
		// Peer reports how much it already has; move into claim.
		Message claim;
		claim.type = MessageType::Claim;
		claim.tx_id = txId;
		claim.payload = { { "name", std::filesystem::path(tx.localPath).filename().string() },
			              { "size", tx.totalSize } };
		_session->send(claim);
		break;
	}
	case MessageType::ClaimAck: {
		if (!msg.payload.value("ok", false))
		{
			abort(txId, msg.payload.value("message", "claim rejected"));
			return;
		}
		tx.nextOffset = msg.payload.value("offset", 0);
		sendChunk(tx);
		break;
	}
	case MessageType::DataAck: {
		if (!msg.payload.value("ok", false))
		{
			abort(txId, "data rejected");
			return;
		}
		tx.nextOffset = msg.payload.value("offset", tx.nextOffset);
		sendChunk(tx);
		break;
	}
	case MessageType::CommitAck: {
		removeOutgoing(txId);
		if (msg.payload.value("ok", false))
			std::cout << "[=] Transfer committed (tx " << txId << ")\n";
		else
			std::cerr << "[!] Commit failed on peer for tx " << txId << " (file lost?)\n";
		break;
	}
	case MessageType::Abort:
		removeOutgoing(txId);
		std::cout << "[~] Peer aborted tx " << txId << ": "
		          << msg.payload.value("reason", "no reason") << '\n';
		break;
	default:
		break;
	}
}

// ---------------------------------------------------------------------------
// Receiver
// ---------------------------------------------------------------------------

void TransactionManager::handleDefault(const Message& msg)
{
	if (msg.type == MessageType::Claim)
		onClaim(msg.tx_id, msg);
	else if (msg.type == MessageType::Status)
		onStatus(msg);
}

void TransactionManager::onStatus(const Message& msg)
{
	const std::string name
	    = std::filesystem::path(msg.payload.value("name", "")).filename().string();
	const uint64_t size = msg.payload.value("size", 0);

	Message ack;
	ack.type = MessageType::StatusAck;
	ack.tx_id = msg.tx_id;
	ack.payload = {
		{ "ok", true }, { "name", name }, { "size", size }, { "offset", resumeOffset(name, size) }
	};
	_session->send(ack);
}

void TransactionManager::onClaim(uint64_t txId, const Message& msg)
{
	namespace fs = std::filesystem;
	const std::string name = fs::path(msg.payload.value("name", "")).filename().string();
	const uint64_t size = msg.payload.value("size", 0);

	fs::path share(_node.getConfig().getPath());
	fs::path stagingDir = share / kStagingDir;
	std::error_code ec;
	fs::create_directories(stagingDir, ec);

	std::string staging = (stagingDir / (name + ".part")).string();
	uint64_t offset = resumeOffset(name, size);

	Incoming in;
	in.stagingPath = staging;
	in.finalPath = (share / name).string();
	in.size = size;
	// Resume an interrupted transfer by appending, otherwise start fresh.
	in.stream.open(staging, (offset > 0 ? std::ios::binary | std::ios::app
	                                    : std::ios::binary | std::ios::trunc));
	if (!in.stream.is_open())
	{
		reply(txId, MessageType::ClaimAck,
		      { { "ok", false }, { "message", "cannot stage destination file" } });
		return;
	}
	_incoming.emplace(txId, std::move(in));

	// Register the receive-side handler for the rest of this transaction.
	_session->onTx(txId, [this, txId](const Message& m) {
		switch (m.type)
		{
		case MessageType::Data:
			handleData(txId, m);
			break;
		case MessageType::Commit:
			finishIncoming(txId, true);
			reply(txId, MessageType::CommitAck, { { "ok", true } });
			break;
		case MessageType::Abort:
			finishIncoming(txId, false);
			break;
		default:
			break;
		}
	});

	reply(txId, MessageType::ClaimAck, { { "ok", true }, { "offset", offset } });
}

uint64_t TransactionManager::resumeOffset(const std::string& name, uint64_t size) const
{
	namespace fs = std::filesystem;
	fs::path staging = fs::path(_node.getConfig().getPath()) / kStagingDir / (name + ".part");
	std::error_code ec;
	uint64_t cur = fs::file_size(staging, ec);
	if (ec)
		return 0;
	return cur < size ? cur : size;
}

void TransactionManager::handleData(uint64_t txId, const Message& msg)
{
	auto it = _incoming.find(txId);
	if (it == _incoming.end())
	{
		reply(txId, MessageType::DataAck, { { "ok", false } });
		return;
	}

	const std::string bytes = base64::decode(msg.payload.value("data", ""));
	it->second.stream.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
	it->second.stream.flush();

	const uint64_t offset = static_cast<uint64_t>(it->second.stream.tellp());
	reply(txId, MessageType::DataAck, { { "ok", true }, { "offset", offset } });
}

void TransactionManager::finishIncoming(uint64_t txId, bool commit)
{
	auto it = _incoming.find(txId);
	if (it == _incoming.end())
		return;

	it->second.stream.close();
	if (commit)
	{
		std::error_code ec;
		std::filesystem::rename(it->second.stagingPath, it->second.finalPath, ec);
		if (ec)
			std::cerr << "[!] Commit rename failed: " << ec.message() << '\n';
		else
			std::cout << "[=] Received and committed " << it->second.finalPath << '\n';
	}
	else
	{
		std::error_code ec;
		std::filesystem::remove(it->second.stagingPath, ec);
		std::cout << "[~] Aborted tx " << txId << ", staged file removed\n";
	}
	_incoming.erase(it);
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

void TransactionManager::reply(uint64_t txId, MessageType type, json payload)
{
	Message msg;
	msg.type = type;
	msg.tx_id = txId;
	msg.payload = std::move(payload);
	_session->send(msg);
}

void TransactionManager::removeOutgoing(uint64_t txId)
{
	_outgoing.erase(txId);
}

void TransactionManager::abort(uint64_t txId, const std::string& reason)
{
	Message abort;
	abort.type = MessageType::Abort;
	abort.tx_id = txId;
	abort.payload = { { "reason", reason } };
	_session->send(abort);
	removeOutgoing(txId);
}
