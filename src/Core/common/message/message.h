#pragma once
#include <cstdint>
#include <string>

#include <nlohmann/json.hpp>

using json = nlohmann::json;

// The set of message types exchanged between nodes over the wire.
enum class MessageType {
	Hello,
	Claim,
	ClaimAck,
	Data,
	DataAck,
	Commit,
	CommitAck,
	Abort,
	Status,
};

// Wire-level message envelope. Every frame exchanged between nodes is a JSON
// object serialized from this struct: { "type", "tx_id", "payload" }.
struct Message
{
	MessageType type;
	uint64_t tx_id;
	json payload;

	json toJson() const;
	static Message fromJson(const json& j);
};

NLOHMANN_JSON_SERIALIZE_ENUM(MessageType, { { MessageType::Hello, "hello" },
                                            { MessageType::Claim, "claim" },
                                            { MessageType::ClaimAck, "claim_ack" },
                                            { MessageType::Data, "data" },
                                            { MessageType::DataAck, "data_ack" },
                                            { MessageType::Commit, "commit" },
                                            { MessageType::CommitAck, "commit_ack" },
                                            { MessageType::Abort, "abort" },
                                            { MessageType::Status, "status" } })

inline json Message::toJson() const
{
	return json{ { "type", type }, { "tx_id", tx_id }, { "payload", payload } };
}

inline Message Message::fromJson(const json& j)
{
	Message msg;
	msg.type = j.at("type").get<MessageType>();
	msg.tx_id = j.at("tx_id").get<uint64_t>();
	msg.payload = j.value("payload", json::object());
	return msg;
}
