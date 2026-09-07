#include <gtest/gtest.h>

#include <nlohmann/json.hpp>

#include "message.h"

using json = nlohmann::json;

namespace {

Message makeMessage(MessageType type, json payload)
{
	Message m;
	m.type = type;
	m.payload = std::move(payload);
	return m;
}

} // namespace

// Round-trip: toJson -> fromJson preserves type and payload for every
// message type used on the wire.
TEST(Message, RoundTripPreservesFields)
{
	const struct
	{
		MessageType type;
		const char *name;
	} cases[] = { { MessageType::Hello, "hello" }, { MessageType::PeersList, "peersList" },
		          { MessageType::Ping, "ping" },   { MessageType::Claim, "claim" },
		          { MessageType::Data, "data" },   { MessageType::Commit, "commit" },
		          { MessageType::Abort, "abort" }, { MessageType::Status, "status" } };

	for (const auto& c : cases)
	{
		Message m = makeMessage(c.type, json::object());
		Message back = Message::fromJson(m.toJson());
		EXPECT_EQ(back.type, c.type) << "type mismatch for " << c.name;
		EXPECT_TRUE(back.payload.is_object());
	}
}

// The wire envelope carries the type as a human-readable string name.
TEST(Message, JsonCarriesStringTypeName)
{
	Message m = makeMessage(MessageType::Hello, json::object());
	json j = m.toJson();
	EXPECT_EQ(j.at("type").get<std::string>(), "hello");
}

// The wire envelope exposes type and payload only; there is no tx_id field.
TEST(Message, JsonHasEnvelopeFields)
{
	Message m = makeMessage(MessageType::Data, json{ { "part", 3 } });
	json j = m.toJson();
	ASSERT_TRUE(j.contains("type"));
	ASSERT_TRUE(j.contains("payload"));
	EXPECT_FALSE(j.contains("tx_id"));
	EXPECT_EQ(j.at("payload").at("part").get<int>(), 3);
}

// Type names serialize back and forth through the JSON string representation.
TEST(Message, EnumNameRoundTrip)
{
	EXPECT_EQ(json(MessageType::Hello).get<std::string>(), "hello");
	EXPECT_EQ(json("peersList").get<MessageType>(), MessageType::PeersList);
	EXPECT_EQ(json("ping").get<MessageType>(), MessageType::Ping);
	EXPECT_EQ(json(MessageType::Ping).get<std::string>(), "ping");
	EXPECT_EQ(json("status").get<MessageType>(), MessageType::Status);
	EXPECT_EQ(json(MessageType::Claim).get<std::string>(), "claim");
	EXPECT_EQ(json("abort").get<MessageType>(), MessageType::Abort);
}

// A frame without a payload field deserializes to an empty JSON object.
TEST(Message, MissingPayloadDefaultsToEmptyObject)
{
	json j = json{ { "type", "commit" } };
	Message m = Message::fromJson(j);
	EXPECT_EQ(m.type, MessageType::Commit);
	EXPECT_TRUE(m.payload.is_object());
	EXPECT_TRUE(m.payload.empty());
}

// A frame with a nested, rich payload survives a round-trip unchanged.
TEST(Message, NestedPayloadRoundTrip)
{
	json payload = json{ { "path", "/tmp/f.bin" },
		                 { "size", 1024 },
		                 { "meta", json{ { "checksum", "abc123" }, { "flags", { 1, 2, 3 } } } } };
	Message m = makeMessage(MessageType::Data, payload);
	Message back = Message::fromJson(m.toJson());
	EXPECT_EQ(back.type, MessageType::Data);
	EXPECT_EQ(back.payload, payload);
}

// A frame with a scalar string payload survives a round-trip.
TEST(Message, StringPayloadRoundTrip)
{
	Message m = makeMessage(MessageType::Hello, "greetings");
	Message back = Message::fromJson(m.toJson());
	EXPECT_EQ(back.payload.get<std::string>(), "greetings");
}

// Missing mandatory type field is surfaced as out_of_range by at().
TEST(Message, MissingTypeThrows)
{
	json j = json{ { "payload", json::object() } };
	EXPECT_THROW(Message::fromJson(j), json::out_of_range);
}
