#include <gtest/gtest.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <vector>

#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/system/error_code.hpp>

#include "node.h"
#include "session.h"

namespace asio = boost::asio;
namespace beast = boost::beast;
namespace websocket = boost::beast::websocket;
using tcp = boost::asio::ip::tcp;
using json = nlohmann::json;

// These tests are built against the keepalive-tuned node (node_keepalive): the
// production PING_INTERVAL (10s) and IDLE_TIMEOUT (20s) — baked into session.h
// as static constexpr from VSNA_* compile definitions — are replaced by short
// values (300ms / 800ms, see CMakeLists.txt) so the behaviour can be exercised
// deterministically without waiting minutes.

namespace {

// Bind to an ephemeral port and return the chosen port number.
uint16_t freePort()
{
	asio::io_context ioc;
	tcp::acceptor acceptor(ioc, tcp::endpoint(asio::ip::make_address("127.0.0.1"), 0));
	return acceptor.local_endpoint().port();
}

// Polls a predicate until it holds or the timeout elapses.
bool waitUntil(const std::function<bool()>& pred,
               std::chrono::seconds timeout = std::chrono::seconds(10))
{
	auto deadline = std::chrono::steady_clock::now() + timeout;
	while (std::chrono::steady_clock::now() < deadline)
	{
		if (pred())
			return true;
		std::this_thread::sleep_for(std::chrono::milliseconds(20));
	}
	return pred();
}

namespace detail {

// Force-closes a websocket at the transport level. A ws close handshake would
// block forever against a peer that keeps the socket open (or never answers),
// so peers tear the socket down directly; the session observes the EOF.
void forceClose(websocket::stream<beast::tcp_stream>& ws)
{
	boost::system::error_code ignore;
	ws.next_layer().socket().shutdown(tcp::socket::shutdown_both, ignore);
	ws.next_layer().socket().close(ignore);
}

} // namespace detail

// A raw WebSocket server for the dial direction. Accepts the dialing session's
// connection, consumes the greeting hello, replies with its own hello, and then
// records every frame the session sends (the periodic keepalive pings) until the
// test stops it.
class KeepaliveServer {
  public:
	explicit KeepaliveServer(uint16_t port) : _port(port)
	{}

	void start()
	{
		_run = std::thread([this] {
			asio::io_context ioc;
			tcp::acceptor acceptor(ioc, tcp::endpoint(asio::ip::make_address("127.0.0.1"), _port));
			acceptor.listen();
			_bound.set_value();

			websocket::stream<beast::tcp_stream> ws(asio::make_strand(ioc));
			beast::get_lowest_layer(ws).expires_after(std::chrono::seconds(10));
			tcp::socket sock = acceptor.accept();
			ws.next_layer().socket() = std::move(sock);
			ws.set_option(websocket::stream_base::timeout{ std::chrono::seconds(10),
			                                               std::chrono::seconds(60) });
			ws.accept();
			ws.text(true);

			beast::flat_buffer greeting;
			ws.read(greeting);
			_greeting = beast::buffers_to_string(greeting.data());

			const std::string hello
			    = json{ { "type", "hello" }, { "payload", json::object() } }.dump();
			ws.write(asio::buffer(hello));

			_ready.set_value();

			while (!_stop.load(std::memory_order_acquire))
			{
				beast::flat_buffer buf;
				boost::system::error_code ec;
				ws.read(buf, ec);
				if (ec)
					break;
				std::lock_guard<std::mutex> lock(_mtx);
				_frames.push_back(beast::buffers_to_string(buf.data()));
				// Mirror a ping back: the session's idle deadline is refreshed
				// only by incoming frames, and this peer is otherwise silent.
				const std::string ping
				    = json{ { "type", "ping" }, { "payload", json::object() } }.dump();
				ws.write(asio::buffer(ping));
			}

			detail::forceClose(ws);
		});
	}

	// Block until the acceptor is listening, so the caller can dial safely.
	void waitBound()
	{
		if (_bound.get_future().wait_for(std::chrono::seconds(5)) != std::future_status::ready)
			throw std::runtime_error("KeepaliveServer: never began listening");
	}

	// Block until the server has sent its hello.
	void waitReady()
	{
		if (_ready.get_future().wait_for(std::chrono::seconds(15)) != std::future_status::ready)
			throw std::runtime_error("KeepaliveServer: greeting/handshake timed out");
	}

	// Number of keepalive ping frames observed from the session.
	std::size_t pingCount()
	{
		std::lock_guard<std::mutex> lock(_mtx);
		return std::count_if(_frames.begin(), _frames.end(), [](const std::string& frame) {
			try
			{
				return json::parse(frame).at("type").get<std::string>() == "ping";
			}
			catch (...)
			{
				return false;
			}
		});
	}

	void stop()
	{
		_stop.store(true, std::memory_order_release);
		if (_run.joinable())
			_run.join();
	}

  private:
	uint16_t _port;
	std::thread _run;
	std::promise<void> _bound;
	std::promise<void> _ready;
	std::atomic<bool> _stop{ false };
	std::mutex _mtx;
	std::vector<std::string> _frames;
	std::string _greeting;
};

// A raw WebSocket client for the accept direction. Connects to the accepting
// NodeSession, consumes the session's greeting hello, sends its own hello, then
// sends arbitrary frames and reads replies/pings with per-read timeout windows.
class KeepaliveClient {
  public:
	explicit KeepaliveClient(uint16_t port) : _port(port)
	{}

	void connect()
	{
		_ws = std::unique_ptr<websocket::stream<beast::tcp_stream>>(
		    new websocket::stream<beast::tcp_stream>(_ioc));
		_ws->set_option(
		    websocket::stream_base::timeout{ std::chrono::seconds(10), std::chrono::seconds(10) });
		_ws->next_layer().connect(tcp::endpoint(asio::ip::make_address("127.0.0.1"), _port));
		_ws->handshake("127.0.0.1:" + std::to_string(_port), "/");
		_ws->text(true);

		beast::flat_buffer greeting;
		_ws->read(greeting);
		_greeting = beast::buffers_to_string(greeting.data());

		const std::string hello = json{ { "type", "hello" }, { "payload", json::object() } }.dump();
		_ws->write(asio::buffer(hello));
	}

	void sendFrame(const std::string& text)
	{
		_ws->write(asio::buffer(text));
	}

	// Read one frame, bounded by Beast's idle timeout. Returns nullopt when the
	// read ends without a frame (idle deadline or the connection died).
	std::optional<std::string> readFrame(std::chrono::milliseconds window)
	{
		_ws->set_option(websocket::stream_base::timeout{ std::chrono::seconds(5), window });
		beast::flat_buffer buf;
		boost::system::error_code ec;
		_ws->read(buf, ec);
		if (ec)
			return std::nullopt;
		return beast::buffers_to_string(buf.data());
	}

	void stop()
	{
		if (_ws)
			detail::forceClose(*_ws);
	}

  private:
	uint16_t _port;
	asio::io_context _ioc;
	std::unique_ptr<websocket::stream<beast::tcp_stream>> _ws;
	std::string _greeting;
};

} // namespace

// Dial direction: after the hello handshake the peer stays silent, and the
// session keeps the connection alive by sending a ping every PING_INTERVAL.
TEST(Keepalive, PingsPeriodicallyWhileIdle)
{
	auto port = freePort();
	asio::io_context ioc;
	Node node;

	KeepaliveServer server(port);
	server.start();
	server.waitBound();

	auto session = std::make_shared<NodeSession>(node, ioc);
	session->dial("127.0.0.1", std::to_string(port));

	std::thread worker([&] {
		asio::executor_work_guard<asio::io_context::executor_type> guard(ioc.get_executor());
		ioc.run();
	});

	server.waitReady();
	EXPECT_TRUE(waitUntil([&] { return server.pingCount() >= 2; }))
	    << "periodic keepalive pings were not observed while idle";

	server.stop();
	ioc.stop();
	worker.join();
}

// Accept direction: an outgoing non-ping frame (the status reply to a claim)
// suppresses the next keepalive ping tick, while pings keep flowing afterwards.
// The suppression is proven by timing: the tick immediately following the reply
// must stay silent, so the first incoming ping may not arrive within one
// PING_INTERVAL of the reply — only at the tick one full interval later.
TEST(Keepalive, SuppressesPingRightAfterOutgoingFrame)
{
	auto port = freePort();
	asio::io_context ioc;
	Node node;

	tcp::acceptor acceptor(ioc, tcp::endpoint(asio::ip::make_address("127.0.0.1"), port));
	acceptor.listen();

	auto session = std::make_shared<NodeSession>(node, ioc);
	session->onMessage([&session](const Message& msg) {
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

	KeepaliveClient client(port);
	client.connect();

	client.sendFrame(json{ { "type", "claim" }, { "payload", json::object() } }.dump());
	std::optional<std::string> reply = client.readFrame(std::chrono::milliseconds(2000));
	ASSERT_TRUE(reply.has_value()) << "no status reply to the claim";
	EXPECT_EQ(json::parse(*reply).at("type").get<std::string>(), "status");

	// The status reply was written just now; the very next tick is suppressed.
	auto replyAt = std::chrono::steady_clock::now();
	std::optional<std::string> ping = client.readFrame(std::chrono::milliseconds(4000));
	ASSERT_TRUE(ping.has_value()) << "keepalive pings stopped after the reply";
	auto delay = std::chrono::duration_cast<std::chrono::milliseconds>(
	                 std::chrono::steady_clock::now() - replyAt)
	                 .count();
	EXPECT_EQ(json::parse(*ping).at("type").get<std::string>(), "ping");
	auto interval = static_cast<long>(NodeSession::PING_INTERVAL.count());
	EXPECT_GE(delay, interval) << "ping arrived within one tick of an outgoing frame";
	EXPECT_LT(delay, 2 * interval) << "assertion window too tight for this environment";

	client.stop();
	ioc.stop();
	worker.join();
}

// Both directions: a peer that goes completely silent (sends nothing) gets
// disconnected by the idle timeout (IDLE_TIMEOUT) instead of sitting forever.
// The session pings in the meantime (that is the keepalive), then must tear the
// connection down itself, so the read started after the ping must fail instead
// of staying open.
TEST(Keepalive, IdleTimeoutTearsDownSilentPeer)
{
	auto port = freePort();
	asio::io_context ioc;
	Node node;

	tcp::acceptor acceptor(ioc, tcp::endpoint(asio::ip::make_address("127.0.0.1"), port));
	acceptor.listen();

	auto session = std::make_shared<NodeSession>(node, ioc);
	std::thread worker([&] {
		tcp::socket sock = acceptor.accept();
		session->accept(std::move(sock));
		asio::executor_work_guard<asio::io_context::executor_type> guard(ioc.get_executor());
		ioc.run();
	});

	std::this_thread::sleep_for(std::chrono::milliseconds(200));

	KeepaliveClient client(port);
	client.connect();

	// Stay silent after the handshake. The session sends at least one periodic
	// ping, and then IdleTimeout (800ms here) closes the connection because no
	// incoming frame has refreshed the deadline.
	std::optional<std::string> ping = client.readFrame(std::chrono::milliseconds(2000));
	ASSERT_TRUE(ping.has_value()) << "expected the session to ping while idle";
	EXPECT_EQ(json::parse(*ping).at("type").get<std::string>(), "ping");

	auto start = std::chrono::steady_clock::now();
	std::optional<std::string> frame = client.readFrame(std::chrono::milliseconds(4000));
	auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
	    std::chrono::steady_clock::now() - start);

	EXPECT_FALSE(frame.has_value()) << "peer stayed connected despite being idle";
	EXPECT_LT(elapsed.count(), 2500) << "idle teardown took too long: " << elapsed.count() << "ms";

	client.stop();
	ioc.stop();
	worker.join();
}