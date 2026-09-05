#pragma once
#include <cstdint>
#include <string>

#include <nlohmann/json.hpp>

using json = nlohmann::json;

// The set of message types exchanged between nodes over the wire.
enum class MessageType {
	Hello,
	Ping,
	Claim,
	Data,
	Commit,
	Abort,
	Status,
};

// Wire-level message envelope. Every frame exchanged between nodes is a JSON
// object serialized from this struct: { "type", "payload" }. A frame is
// identified solely by its type; there is no transaction id.
struct Message
{
	MessageType type;
	json payload;

	json toJson() const;
	static Message fromJson(const json& j);
};

NLOHMANN_JSON_SERIALIZE_ENUM(MessageType, { { MessageType::Hello, "hello" },
                                            { MessageType::Ping, "ping" },
                                            { MessageType::Claim, "claim" },
                                            { MessageType::Data, "data" },
                                            { MessageType::Commit, "commit" },
                                            { MessageType::Abort, "abort" },
                                            { MessageType::Status, "status" } })

inline json Message::toJson() const
{
	return json{ { "type", type }, { "payload", payload } };
}

inline Message Message::fromJson(const json& j)
{
	Message msg;
	msg.type = j.at("type").get<MessageType>();
	msg.payload = j.value("payload", json::object());
	return msg;
}
