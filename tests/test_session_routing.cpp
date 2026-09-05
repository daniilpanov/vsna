#include <gtest/gtest.h>

#include <chrono>
#include <cstdint>
#include <future>
#include <memory>
#include <string>
#include <thread>

#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/system/error_code.hpp>

#include <nlohmann/json.hpp>

#include "node.h"

namespace asio = boost::asio;
namespace beast = boost::beast;
namespace websocket = boost::beast::websocket;
using tcp = boost::asio::ip::tcp;
using json = nlohmann::json;

namespace {

// Bind to an ephemeral port and return the chosen port number.
uint16_t freePort()
{
	asio::io_context ioc;
	tcp::acceptor acceptor(ioc, tcp::endpoint(asio::ip::make_address("127.0.0.1"), 0));
	return acceptor.local_endpoint().port();
}

// Waits for a future to become ready, returns false on timeout.
bool ready(const std::future<std::string>& f,
           std::chrono::seconds timeout = std::chrono::seconds(10))
{
	return f.wait_for(timeout) == std::future_status::ready;
}

// A raw WebSocket server that binds a port, waits for a connection, sends a JSON
// frame, then reads whatever the peer replies. Exposes promises for the bind and
// the received reply.
class RawServerSendsThenReads {
  public:
	RawServerSendsThenReads(uint16_t port, std::string frame)
	    : _port(port), _frame(std::move(frame)), _bound(std::promise<std::string>())
	{}

	void start()
	{
		_thread = std::thread([this] {
			asio::io_context ioc;
			tcp::acceptor acceptor(ioc, tcp::endpoint(asio::ip::make_address("127.0.0.1"), _port));
			acceptor.listen();
			_bound.set_value("listening");

			websocket::stream<beast::tcp_stream> ws(asio::make_strand(ioc));
			beast::get_lowest_layer(ws).expires_after(std::chrono::seconds(10));
			tcp::socket sock = acceptor.accept();
			ws.next_layer().socket() = std::move(sock);

			ws.set_option(websocket::stream_base::timeout::suggested(beast::role_type::server));
			ws.accept();

			ws.text(true);
			ws.write(asio::buffer(_frame));

			beast::flat_buffer buffer;
			ws.read(buffer);
			_reply.set_value(beast::buffers_to_string(buffer.data()));

			// Force-close: the peer session stays open (persistent), so a
			// websocket close handshake would block forever.
			boost::system::error_code ignore;
			ws.next_layer().socket().shutdown(tcp::socket::shutdown_both, ignore);
			ws.next_layer().socket().close(ignore);
		});
	}

	bool waitBound()
	{
		return _bound.get_future().wait_for(std::chrono::seconds(5)) == std::future_status::ready;
	}

	std::string result()
	{
		return _reply.get_future().get();
	}

	void join()
	{
		if (_thread.joinable())
			_thread.join();
	}

  private:
	uint16_t _port;
	std::string _frame;
	std::thread _thread;
	std::promise<std::string> _bound;
	std::promise<std::string> _reply;
};

// A raw WebSocket client that dials the NodeSession (accept direction), sends a
// JSON frame, then reads the peer's reply.
class RawClientSendsThenReads {
  public:
	explicit RawClientSendsThenReads(uint16_t port, std::string frame)
	    : _port(port), _frame(std::move(frame))
	{}

	std::string run()
	{
		asio::io_context ioc;
		websocket::stream<beast::tcp_stream> ws(asio::make_strand(ioc));

		beast::get_lowest_layer(ws).expires_after(std::chrono::seconds(10));
		beast::get_lowest_layer(ws).connect(
		    tcp::endpoint(asio::ip::make_address("127.0.0.1"), _port));
		beast::get_lowest_layer(ws).expires_never();
		ws.set_option(websocket::stream_base::timeout::suggested(beast::role_type::client));
		ws.handshake("127.0.0.1", "/");

		ws.text(true);
		ws.write(asio::buffer(_frame));

		beast::flat_buffer buffer;
		ws.read(buffer);
		std::string reply = beast::buffers_to_string(buffer.data());

		// Force-close: the peer session stays open (persistent), so a websocket
		// close handshake would block forever.
		boost::system::error_code ignore;
		ws.next_layer().socket().shutdown(tcp::socket::shutdown_both, ignore);
		ws.next_layer().socket().close(ignore);
		return reply;
	}

  private:
	uint16_t _port;
	std::string _frame;
};

// A raw WebSocket client that sends two frames then reads one reply.
// Used to test that a control frame is ignored and the next valid frame routes.
class RawClientSendsTwoFramesThenReads {
  public:
	explicit RawClientSendsTwoFramesThenReads(uint16_t port, std::string first, std::string second)
	    : _port(port), _first(std::move(first)), _second(std::move(second))
	{}

	std::string run()
	{
		asio::io_context ioc;
		websocket::stream<beast::tcp_stream> ws(asio::make_strand(ioc));

		beast::get_lowest_layer(ws).expires_after(std::chrono::seconds(10));
		beast::get_lowest_layer(ws).connect(
		    tcp::endpoint(asio::ip::make_address("127.0.0.1"), _port));
		beast::get_lowest_layer(ws).expires_never();
		ws.set_option(websocket::stream_base::timeout::suggested(beast::role_type::client));
		ws.handshake("127.0.0.1", "/");

		ws.text(true);
		ws.write(asio::buffer(_first));
		ws.write(asio::buffer(_second));

		beast::flat_buffer buffer;
		ws.read(buffer);
		std::string reply = beast::buffers_to_string(buffer.data());

		boost::system::error_code ignore;
		ws.next_layer().socket().shutdown(tcp::socket::shutdown_both, ignore);
		ws.next_layer().socket().close(ignore);
		return reply;
	}

  private:
	uint16_t _port;
	std::string _first;
	std::string _second;
};

// A raw WebSocket server that sends two frames then reads one reply.
// Used to test the dial direction: a control frame from the server is swallowed
// while a following valid frame still routes and replies.
class RawServerSendsTwoFramesThenReads {
  public:
	RawServerSendsTwoFramesThenReads(uint16_t port, std::string first, std::string second)
	    : _port(port), _first(std::move(first)), _second(std::move(second))
	{}

	void start()
	{
		_thread = std::thread([this] {
			asio::io_context ioc;
			tcp::acceptor acceptor(ioc, tcp::endpoint(asio::ip::make_address("127.0.0.1"), _port));
			acceptor.listen();
			_bound.set_value("listening");

			websocket::stream<beast::tcp_stream> ws(asio::make_strand(ioc));
			beast::get_lowest_layer(ws).expires_after(std::chrono::seconds(10));
			tcp::socket sock = acceptor.accept();
			ws.next_layer().socket() = std::move(sock);

			ws.set_option(websocket::stream_base::timeout::suggested(beast::role_type::server));
			ws.accept();

			ws.text(true);
			ws.write(asio::buffer(_first));
			ws.write(asio::buffer(_second));

			beast::flat_buffer buffer;
			ws.read(buffer);
			_reply.set_value(beast::buffers_to_string(buffer.data()));

			boost::system::error_code ignore;
			ws.next_layer().socket().shutdown(tcp::socket::shutdown_both, ignore);
			ws.next_layer().socket().close(ignore);
		});
	}

	bool waitBound()
	{
		return _bound.get_future().wait_for(std::chrono::seconds(5)) == std::future_status::ready;
	}

	std::string result()
	{
		return _reply.get_future().get();
	}

	void join()
	{
		if (_thread.joinable())
			_thread.join();
	}

  private:
	uint16_t _port;
	std::string _first;
	std::string _second;
	std::thread _thread;
	std::promise<std::string> _bound;
	std::promise<std::string> _reply;
};

} // namespace

// Dial direction: a NodeSession dials a raw WebSocket server. The server sends a
// JSON frame; the session's type handler routes it and replies via send(); the
// server verifies the reply. Proves send(), persistence and type routing.
TEST(PersistentSessionDial, RoutesMessageAndReplies)
{
	auto port = freePort();
	RawServerSendsThenReads server(
	    port, json{ { "type", "hello" }, { "payload", json::object() } }.dump());
	server.start();
	ASSERT_TRUE(server.waitBound()) << "server did not start listening";

	std::promise<std::string> gotP;
	auto got = gotP.get_future();

	Node node;
	asio::io_context ioc;
	auto session = std::make_shared<NodeSession>(node, ioc);

	session->onType(MessageType::Hello, [&session, &gotP](const Message& msg) {
		gotP.set_value(json(msg.type).get<std::string>());
		Message reply;
		reply.type = MessageType::Status;
		reply.payload = json::object();
		session->send(reply);
	});

	session->dial("127.0.0.1", std::to_string(port));

	std::thread worker([&ioc] {
		asio::executor_work_guard<asio::io_context::executor_type> guard(ioc.get_executor());
		ioc.run();
	});

	ASSERT_TRUE(ready(got)) << "type handler did not fire";

	std::string reply = server.result();
	json j = json::parse(reply);
	EXPECT_EQ(j.at("type").get<std::string>(), "status");

	ioc.stop();
	worker.join();
	server.join();
}

// Accept direction: a NodeSession acts as the websocket server and a raw client
// dials it. The client sends a frame; the session's default handler routes it and
// replies. Proves the symmetric accept path and default (onMessage) routing.
TEST(PersistentSessionAccept, RoutesViaDefaultHandler)
{
	auto port = freePort();
	Node node;
	asio::io_context ioc;

	tcp::acceptor acceptor(ioc, tcp::endpoint(asio::ip::make_address("127.0.0.1"), port));
	acceptor.listen();

	std::promise<std::string> gotP;
	auto got = gotP.get_future();

	auto session = std::make_shared<NodeSession>(node, ioc);
	session->onMessage([&session, &gotP](const Message& msg) {
		gotP.set_value(json(msg.type).get<std::string>());
		Message reply;
		reply.type = MessageType::Status;
		reply.payload = json::object();
		session->send(reply);
	});

	// Block until a connection arrives, hand it to the session, then run the
	// io_context so the session's async reads/writes proceed.
	std::thread worker([&] {
		tcp::socket sock = acceptor.accept();
		session->accept(std::move(sock));
		asio::executor_work_guard<asio::io_context::executor_type> guard(ioc.get_executor());
		ioc.run();
	});

	// Give the listener a moment to come up, then a raw client dials it.
	std::this_thread::sleep_for(std::chrono::milliseconds(200));

	RawClientSendsThenReads client(
	    port, json{ { "type", "hello" }, { "payload", json::object() } }.dump());
	std::string replyText;
	std::string clientErr;
	std::thread clientThread([&] {
		try
		{
			replyText = client.run();
		}
		catch (const std::exception& e)
		{
			clientErr = e.what();
		}
	});

	ASSERT_TRUE(ready(got)) << "default handler did not fire";

	clientThread.join();
	EXPECT_TRUE(clientErr.empty()) << "client error: " << clientErr;
	json j = json::parse(replyText);
	EXPECT_EQ(j.at("type").get<std::string>(), "status");

	ioc.stop();
	worker.join();
}

// Accept direction: a raw client sends a garbage frame followed by a valid JSON
// frame. The session ignores the garbage and routes the valid message. Proves
// that on_read survives non-JSON input without breaking the connection.
TEST(PersistentSessionAccept, InvalidJSONSessionStaysAlive)
{
	auto port = freePort();
	Node node;
	asio::io_context ioc;

	tcp::acceptor acceptor(ioc, tcp::endpoint(asio::ip::make_address("127.0.0.1"), port));
	acceptor.listen();

	std::promise<std::string> gotP;
	auto got = gotP.get_future();

	auto session = std::make_shared<NodeSession>(node, ioc);
	session->onMessage([&session, &gotP](const Message& msg) {
		gotP.set_value(json(msg.type).get<std::string>());
		Message reply;
		reply.type = MessageType::Status;
		reply.payload = json::object();
		session->send(reply);
	});

	std::thread worker([&] {
		tcp::socket sock = acceptor.accept();
		session->accept(std::move(sock));
		asio::executor_work_guard<asio::io_context::executor_type> guard(ioc.get_executor());
		ioc.run();
	});

	std::this_thread::sleep_for(std::chrono::milliseconds(200));

	std::string garbage = "this is not json {{{";
	std::string valid = json{ { "type", "hello" }, { "payload", json::object() } }.dump();

	RawClientSendsTwoFramesThenReads client(port, std::move(garbage), std::move(valid));
	std::string replyText;
	std::string clientErr;
	std::thread clientThread([&] {
		try
		{
			replyText = client.run();
		}
		catch (const std::exception& e)
		{
			clientErr = e.what();
		}
	});

	ASSERT_TRUE(ready(got)) << "default handler did not fire after garbage frame";

	clientThread.join();
	EXPECT_TRUE(clientErr.empty()) << "client error: " << clientErr;
	json j = json::parse(replyText);
	EXPECT_EQ(j.at("type").get<std::string>(), "status");

	ioc.stop();
	worker.join();
}

// Accept direction: a valid message with no payload field (payload defaults to
// null/empty). Proves that missing payload does not crash the session.
TEST(PersistentSessionAccept, EmptyPayloadRoutedCorrectly)
{
	auto port = freePort();
	Node node;
	asio::io_context ioc;

	tcp::acceptor acceptor(ioc, tcp::endpoint(asio::ip::make_address("127.0.0.1"), port));
	acceptor.listen();

	std::promise<std::string> gotP;
	auto got = gotP.get_future();

	auto session = std::make_shared<NodeSession>(node, ioc);
	session->onMessage([&session, &gotP](const Message& msg) {
		gotP.set_value(json(msg.type).get<std::string>());
		Message reply;
		reply.type = MessageType::Status;
		reply.payload = json::object();
		session->send(reply);
	});

	std::thread worker([&] {
		tcp::socket sock = acceptor.accept();
		session->accept(std::move(sock));
		asio::executor_work_guard<asio::io_context::executor_type> guard(ioc.get_executor());
		ioc.run();
	});

	std::this_thread::sleep_for(std::chrono::milliseconds(200));

	// Frame with no payload key — fromJson defaults to json::object().
	std::string frame = json{ { "type", "hello" } }.dump();

	RawClientSendsThenReads client(port, std::move(frame));
	std::string replyText;
	std::string clientErr;
	std::thread clientThread([&] {
		try
		{
			replyText = client.run();
		}
		catch (const std::exception& e)
		{
			clientErr = e.what();
		}
	});

	ASSERT_TRUE(ready(got)) << "default handler did not fire with empty payload";

	clientThread.join();
	EXPECT_TRUE(clientErr.empty()) << "client error: " << clientErr;
	json j = json::parse(replyText);
	EXPECT_EQ(j.at("type").get<std::string>(), "status");

	ioc.stop();
	worker.join();
}

// Accept direction: a raw client sends a ping followed by a normal claim frame.
// The ping (a control frame) is swallowed by on_read and never reaches the user
// handler; only the claim is routed and replied to. Proves a ping does not break
// routing or the connection.
TEST(PersistentSession, IncomingPingThenDataStillRoutes)
{
	auto port = freePort();
	Node node;
	asio::io_context ioc;

	tcp::acceptor acceptor(ioc, tcp::endpoint(asio::ip::make_address("127.0.0.1"), port));
	acceptor.listen();

	std::promise<std::string> gotP;
	auto got = gotP.get_future();

	auto session = std::make_shared<NodeSession>(node, ioc);
	session->onType(MessageType::Claim, [&session, &gotP](const Message& msg) {
		gotP.set_value(json(msg.type).get<std::string>());
		Message reply;
		reply.type = MessageType::Status;
		reply.payload = json::object();
		session->send(reply);
	});

	std::thread worker([&] {
		tcp::socket sock = acceptor.accept();
		session->accept(std::move(sock));
		asio::executor_work_guard<asio::io_context::executor_type> guard(ioc.get_executor());
		ioc.run();
	});

	std::this_thread::sleep_for(std::chrono::milliseconds(200));

	std::string ping = json{ { "type", "ping" }, { "payload", json::object() } }.dump();
	std::string claim = json{ { "type", "claim" }, { "payload", json::object() } }.dump();

	RawClientSendsTwoFramesThenReads client(port, std::move(ping), std::move(claim));
	std::string replyText;
	std::string clientErr;
	std::thread clientThread([&] {
		try
		{
			replyText = client.run();
		}
		catch (const std::exception& e)
		{
			clientErr = e.what();
		}
	});

	ASSERT_TRUE(ready(got)) << "claim handler did not fire after a preceding ping";

	clientThread.join();
	EXPECT_TRUE(clientErr.empty()) << "client error: " << clientErr;
	json j = json::parse(replyText);
	EXPECT_EQ(j.at("type").get<std::string>(), "status");

	ioc.stop();
	worker.join();
}

// Dial direction: the dialed NodeSession receives a ping followed by a claim from
// the raw server. The ping is swallowed and the claim still routes and replies.
TEST(PersistentSessionDial, IncomingPingIgnored)
{
	auto port = freePort();
	RawServerSendsTwoFramesThenReads server(
	    port, json{ { "type", "ping" }, { "payload", json::object() } }.dump(),
	    json{ { "type", "claim" }, { "payload", json::object() } }.dump());
	server.start();
	ASSERT_TRUE(server.waitBound()) << "server did not start listening";

	std::promise<std::string> gotP;
	auto got = gotP.get_future();

	Node node;
	asio::io_context ioc;
	auto session = std::make_shared<NodeSession>(node, ioc);
	session->onType(MessageType::Claim, [&session, &gotP](const Message& msg) {
		gotP.set_value(json(msg.type).get<std::string>());
		Message reply;
		reply.type = MessageType::Status;
		reply.payload = json::object();
		session->send(reply);
	});

	session->dial("127.0.0.1", std::to_string(port));

	std::thread worker([&ioc] {
		asio::executor_work_guard<asio::io_context::executor_type> guard(ioc.get_executor());
		ioc.run();
	});

	ASSERT_TRUE(ready(got)) << "claim handler did not fire after a dial-direction ping";

	std::string reply = server.result();
	json j = json::parse(reply);
	EXPECT_EQ(j.at("type").get<std::string>(), "status");

	ioc.stop();
	worker.join();
	server.join();
}

// TODO(core-feature-testing): The heart of the keepalive feature — the outbound
// periodic ping (PING_INTERVAL) and the idle-teardown (IDLE_TIMEOUT) — is baked
// into the shared `node` static lib as compile-time static constexpr (10s / 20s,
// see session.h), so it cannot be exercised by a fast, deterministic unit test
// without a separate test-only `node` target compiled with small constants (e.g.
// VSNA_PING_INTERVAL_MS=300, VSNA_IDLE_TIMEOUT_MS=800) that vsna_tests links
// against instead of `node`. Figure out how to implement that, then cover: the
// periodic ping while idle, a ping being skipped after an outgoing non-ping frame
// (`_ping_suppressed`), and the idle timeout closing the connection.
