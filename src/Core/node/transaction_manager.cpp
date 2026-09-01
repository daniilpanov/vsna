#include "transaction_manager.h"

#include <filesystem>

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

void TransactionManager::sendFile(const std::string& localPath)
{
	auto self = shared_from_this();
	asio::post(_session->strand(), [self, localPath] { self->do_sendFile(localPath); });
}

void TransactionManager::do_sendFile(const std::string& localPath)
{
	uint64_t txId = _nextId++;

	// Register the per-transaction handler that receives the transfer replies
	// (claim_ack, data_ack, commit_ack, abort) for this tx_id.
	_session->onTx(txId, [this, txId](const Message& msg) { handleReply(txId, msg); });

	_outgoing.emplace(txId, Outgoing{ txId, localPath, 0 });

	namespace fs = std::filesystem;
	std::error_code ec;
	auto size = fs::file_size(localPath, ec);
	if (ec)
	{
		std::cerr << "[!] Cannot stat " << localPath << ": " << ec.message() << '\n';
		abort(txId, "cannot stat local file");
		return;
	}

	do_claim(txId, localPath);
}

void TransactionManager::do_claim(uint64_t txId, const std::string& localPath)
{
	Message claim;
	claim.type = MessageType::Claim;
	claim.tx_id = txId;
	claim.payload = { { "name", std::filesystem::path(localPath).filename().string() },
		              { "size", std::filesystem::file_size(localPath) } };
	_session->send(claim);
}

void TransactionManager::handleReply(uint64_t txId, const Message& msg)
{
	auto it = _outgoing.find(txId);
	if (it == _outgoing.end())
		return;
	Outgoing& tx = it->second;

	switch (msg.type)
	{
	case MessageType::ClaimAck: {
		bool ok = msg.payload.value("ok", false);
		if (!ok)
		{
			std::cerr << "[!] Peer rejected claim: "
			          << msg.payload.value("message", "no reason given") << '\n';
			abort(txId, "claim rejected");
			return;
		}
		// Read the whole file, base64-encode it and send it as one data frame.
		// Chunking/ACK-per-chunk and resume land with the chunked-transfer issue.
		std::ifstream in(tx.localPath, std::ios::binary);
		if (!in)
		{
			abort(txId, "cannot open local file");
			return;
		}
		std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());

		Message data;
		data.type = MessageType::Data;
		data.tx_id = txId;
		data.payload = { { "data", base64::encode(content) } };
		_session->send(data);
		tx.phase = 1;
		break;
	}
	case MessageType::DataAck: {
		if (msg.payload.value("ok", false))
		{
			Message commit;
			commit.type = MessageType::Commit;
			commit.tx_id = txId;
			_session->send(commit);
			tx.phase = 2;
		}
		else
		{
			abort(txId, "data rejected");
		}
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

void TransactionManager::handleClaim(const Message& msg)
{
	uint64_t txId = msg.tx_id;

	if (!stageClaim(txId, msg))
	{
		reply(txId, MessageType::ClaimAck,
		      { { "ok", false }, { "message", "cannot stage destination file" } });
		return;
	}

	// Register the receiver-side handler for the remaining messages of this
	// transaction (data, commit, abort).
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

	reply(txId, MessageType::ClaimAck, { { "ok", true } });
}

bool TransactionManager::stageClaim(uint64_t txId, const Message& msg)
{
	namespace fs = std::filesystem;
	fs::path share(_node.getConfig().getPath());
	fs::path staging = share / kStagingDir;
	std::error_code ec;
	fs::create_directories(staging, ec);

	Incoming in;
	in.stagingPath = (staging / (std::to_string(txId) + ".part")).string();
	in.finalPath = (share / msg.payload.value("name", "unnamed")).string();
	in.stream.open(in.stagingPath, std::ios::binary | std::ios::trunc);
	if (!in.stream.is_open())
		return false;

	_incoming.emplace(txId, std::move(in));
	return true;
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

	reply(txId, MessageType::DataAck, { { "ok", true } });
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

void TransactionManager::abort(uint64_t txId, const std::string& reason)
{
	Message abort;
	abort.type = MessageType::Abort;
	abort.tx_id = txId;
	abort.payload = { { "reason", reason } };
	_session->send(abort);
	removeOutgoing(txId);
}

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
