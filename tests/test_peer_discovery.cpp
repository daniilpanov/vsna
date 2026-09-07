#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <functional>
#include <future>
#include <map>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/system/error_code.hpp>

#include <nlohmann/json.hpp>

#include "node.h"
#include "peer_registry.h"
#include "session.h"

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

// Returns a copy of the node's default self address, used to prove that a node
// never registers itself as a known peer.
std::string selfAddr()
{
	return Addr().toString();
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

// --- Hello-handshake wire helpers -------------------------------------------

namespace detail {

// Applies a short idle timeout so a websocket read that never receives data
// fails fast instead of blocking forever. Reads for the greeting and reply must
// not hang the whole test when the peer stalls.
void applyIdleTimeout(websocket::stream<beast::tcp_stream>& ws,
                      std::chrono::seconds idle = std::chrono::seconds(5))
{
	websocket::stream_base::timeout opt;
	opt.handshake_timeout = std::chrono::seconds(5);
	opt.idle_timeout = idle;
	ws.set_option(opt);
}

uint16_t probe_idle()
{
	return static_cast<uint16_t>(NodeSession::HELLO_RETRIES) * 10 + 10;
}

// Force-closes a websocket at the transport level. A ws close handshake would
// block forever against the persistent peer session, so peers tear the socket
// down directly; the session observes the EOF in on_read.
void forceClose(websocket::stream<beast::tcp_stream>& ws)
{
	boost::system::error_code ignore;
	ws.next_layer().socket().shutdown(tcp::socket::shutdown_both, ignore);
	ws.next_layer().socket().close(ignore);
}

} // namespace detail

// A raw WebSocket client. Connects to the accepting NodeSession, reads the
// session's hello greeting (the first frame), then sends its own hello (the
// initialization signal) followed by a peersList frame announcing the given
// peer map. The connection is held open until stop() is called.
class RawHelloClient {
  public:
	explicit RawHelloClient(uint16_t port, std::map<std::string, bool> peers, int extraHellos = 0,
	                        std::string announcedAddr = "")
	    : _port(port), _peers(std::move(peers)), _extraHellos(extraHellos),
	      _announcedAddr(std::move(announcedAddr))
	{}

	// The greeting captured from the session and this client's local address as
	// seen over the socket (ip:port).
	struct Result
	{
		std::string greeting;
		std::string localAddr;
	};

	// Launch the background connect/hello exchange and block until the hello has
	// been sent, returning the captured result. The connection stays open.
	Result waitHandshake()
	{
		_run = std::thread([this] {
			asio::io_context ioc;
			websocket::stream<beast::tcp_stream> ws(asio::make_strand(ioc));
			beast::get_lowest_layer(ws).expires_after(std::chrono::seconds(10));
			beast::get_lowest_layer(ws).connect(
			    tcp::endpoint(asio::ip::make_address("127.0.0.1"), _port));
			beast::get_lowest_layer(ws).expires_never();
			detail::applyIdleTimeout(ws);
			ws.handshake("127.0.0.1", "/");

			Result r;
			r.localAddr = ws.next_layer().socket().local_endpoint().address().to_string() + ":"
			              + std::to_string(ws.next_layer().socket().local_endpoint().port());

			ws.text(true);
			beast::flat_buffer greeting;
			ws.read(greeting);
			r.greeting = beast::buffers_to_string(greeting.data());

			const json helloPayload
			    = _announcedAddr.empty() ? json::object() : json{ { "addr", _announcedAddr } };
			const std::string hello
			    = json{ { "type", "hello" }, { "payload", helloPayload } }.dump();
			ws.write(asio::buffer(hello));

			const std::string peersList = json{
				{ "type", "peersList" }, { "payload", json{ { "peers", _peers } } }
			}.dump();
			ws.write(asio::buffer(peersList));

			for (int i = 0; i < _extraHellos; ++i)
				ws.write(asio::buffer(hello));

			_result = r;
			_ready.set_value();

			while (!_stop.load(std::memory_order_acquire))
				std::this_thread::sleep_for(std::chrono::milliseconds(10));

			detail::forceClose(ws);
		});

		if (_ready.get_future().wait_for(std::chrono::seconds(15)) != std::future_status::ready)
			throw std::runtime_error("RawHelloClient: greeting/handshake timed out");
		return _result.value();
	}

	// Close the connection and wait for the background thread to finish.
	void stop()
	{
		_stop.store(true, std::memory_order_release);
		if (_run.joinable())
			_run.join();
	}

  private:
	uint16_t _port;
	std::map<std::string, bool> _peers;
	int _extraHellos;
	std::string _announcedAddr;
	std::thread _run;
	std::promise<void> _ready;
	std::optional<Result> _result;
	std::atomic<bool> _stop{ false };
};

// A raw WebSocket server that the NodeSession dials. Reads the session's hello
// greeting, replies with its own hello, sends a peersList frame, then holds the
// connection open until stop() is called.
class RawHelloServer {
  public:
	explicit RawHelloServer(uint16_t port, std::map<std::string, bool> peers)
	    : _port(port), _peers(std::move(peers))
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
			detail::applyIdleTimeout(ws);
			ws.accept();

			ws.text(true);
			beast::flat_buffer greeting;
			ws.read(greeting);

			const std::string hello
			    = json{ { "type", "hello" }, { "payload", json::object() } }.dump();
			ws.write(asio::buffer(hello));

			const std::string peersList = json{
				{ "type", "peersList" }, { "payload", json{ { "peers", _peers } } }
			}.dump();
			ws.write(asio::buffer(peersList));

			hello_sent();
			while (!_stop.load(std::memory_order_acquire))
				std::this_thread::sleep_for(std::chrono::milliseconds(10));

			detail::forceClose(ws);
		});
	}

	// Block until the acceptor is listening, so the caller can dial safely.
	void waitBound()
	{
		if (_bound.get_future().wait_for(std::chrono::seconds(5)) != std::future_status::ready)
			throw std::runtime_error("RawHelloServer: never began listening");
	}

	// Block until the server has sent its hello.
	void waitHelloSent()
	{
		if (_ready.get_future().wait_for(std::chrono::seconds(15)) != std::future_status::ready)
			throw std::runtime_error("RawHelloServer: greeting/handshake timed out");
	}

	void stop()
	{
		_stop.store(true, std::memory_order_release);
		if (_run.joinable())
			_run.join();
	}

  private:
	void hello_sent()
	{
		_ready.set_value();
	}

	uint16_t _port;
	std::map<std::string, bool> _peers;
	std::thread _run;
	std::promise<void> _ready;
	std::promise<void> _bound;
	std::atomic<bool> _stop{ false };
};

// A raw WebSocket client that dials an accepting NodeSession, consumes the
// session's greeting, and then either sends a first response frame or sends
// nothing. It verifies that the session enforces the hello gate by rejecting
// the connection (observing a close/error on the next read) within a deadline.
class RawProbeClient {
  public:
	RawProbeClient(uint16_t port, std::string firstResponse)
	    : _port(port), _firstResponse(std::move(firstResponse))
	{}

	// Returns true if the session closed the connection as expected.
	bool run()
	{
		return runImpl(false).first;
	}

	// Returns the number of hello greets received from the session before it
	// closed the connection, or -1 if an unexpected non-hello frame arrived.
	int hellosBeforeClose()
	{
		return runImpl(true).second;
	}

  private:
	// Shared runner. When counting, returns the number of hello frames observed
	// before the session closes; otherwise returns just the close-success flag.
	std::pair<bool, int> runImpl(bool count)
	{
		std::promise<std::pair<bool, int>> closedP;
		auto closed = closedP.get_future();
		_run = std::thread([this, &closedP] {
			asio::io_context ioc;
			websocket::stream<beast::tcp_stream> ws(asio::make_strand(ioc));
			beast::get_lowest_layer(ws).expires_after(std::chrono::seconds(10));
			beast::get_lowest_layer(ws).connect(
			    tcp::endpoint(asio::ip::make_address("127.0.0.1"), _port));
			beast::get_lowest_layer(ws).expires_never();
			detail::applyIdleTimeout(ws, std::chrono::seconds(detail::probe_idle()));
			ws.handshake("127.0.0.1", "/");

			ws.text(true);
			beast::flat_buffer greeting;
			ws.read(greeting);

			if (!_firstResponse.empty())
				ws.write(asio::buffer(_firstResponse));

			// Keep reading. The session may retransmit hello greets while it
			// waits; only an actual close (read error) counts as the session
			// rejecting the connection. Any unexpected non-hello data frame means
			// the session failed to reject us.
			int hellos = 0;
			int ok = 0;
			for (int i = 0; i < 1024; ++i)
			{
				beast::flat_buffer buf;
				boost::system::error_code ec;
				ws.read(buf, ec);
				if (ec)
				{
					ok = 1;
					break;
				}
				json j = json::parse(beast::buffers_to_string(buf.data()), nullptr, false);
				if (j.is_discarded() || j.at("type").get<std::string>() != "hello")
				{
					ok = -1;
					break;
				}
				++hellos;
			}
			closedP.set_value({ ok == 1, hellos });
		});

		auto [ok, hellos] = closed.get();
		(void)hellos;
		if (_run.joinable())
			_run.join();
		return count ? std::make_pair(ok == 1, hellos) : std::make_pair(ok == 1, 0);
	}

  private:
	uint16_t _port;
	std::string _firstResponse;
	std::thread _run;
};

// A raw WebSocket client that completes the hello handshake and then sends a
// sequence of peersList frames, one after the other, with a small delay between
// them. Signals readiness once the last peersList has been written.
class RawPeersListClient {
  public:
	RawPeersListClient(uint16_t port, std::vector<std::map<std::string, bool>> updates)
	    : _port(port), _updates(std::move(updates))
	{}

	// Launch the background exchange and block until all peersList frames were
	// sent. Returns once they are queued on the wire.
	void run()
	{
		_run = std::thread([this] {
			asio::io_context ioc;
			websocket::stream<beast::tcp_stream> ws(asio::make_strand(ioc));
			beast::get_lowest_layer(ws).expires_after(std::chrono::seconds(10));
			beast::get_lowest_layer(ws).connect(
			    tcp::endpoint(asio::ip::make_address("127.0.0.1"), _port));
			beast::get_lowest_layer(ws).expires_never();
			detail::applyIdleTimeout(ws);
			ws.handshake("127.0.0.1", "/");

			ws.text(true);
			beast::flat_buffer greeting;
			ws.read(greeting);

			const std::string hello
			    = json{ { "type", "hello" }, { "payload", json::object() } }.dump();
			ws.write(asio::buffer(hello));

			for (const auto& update : _updates)
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
				ws.write(asio::buffer(
				    json{ { "type", "peersList" }, { "payload", json{ { "peers", update } } } }
				        .dump()));
			}

			_sent.set_value();

			while (!_stop.load(std::memory_order_acquire))
				std::this_thread::sleep_for(std::chrono::milliseconds(10));

			detail::forceClose(ws);
		});

		if (_sent.get_future().wait_for(std::chrono::seconds(15)) != std::future_status::ready)
			throw std::runtime_error("RawPeersListClient: timed out");
	}

	void stop()
	{
		_stop.store(true, std::memory_order_release);
		if (_run.joinable())
			_run.join();
	}

  private:
	uint16_t _port;
	std::vector<std::map<std::string, bool>> _updates;
	std::thread _run;
	std::promise<void> _sent;
	std::atomic<bool> _stop{ false };
};

// A raw WebSocket client that completes the hello handshake and then sends a
// list of pre-serialized frames. Signals readiness once all frames were written
// and holds the connection open until stop(). Lets a test inject malformed
// frames (e.g. a peersList with a non-boolean flag) that must not break the
// session.
class RawFramesClient {
  public:
	explicit RawFramesClient(uint16_t port, std::vector<std::string> frames)
	    : _port(port), _frames(std::move(frames))
	{}

	// Launch the background exchange and block until all frames were sent.
	void run()
	{
		_run = std::thread([this] {
			asio::io_context ioc;
			websocket::stream<beast::tcp_stream> ws(asio::make_strand(ioc));
			beast::get_lowest_layer(ws).expires_after(std::chrono::seconds(10));
			beast::get_lowest_layer(ws).connect(
			    tcp::endpoint(asio::ip::make_address("127.0.0.1"), _port));
			beast::get_lowest_layer(ws).expires_never();
			detail::applyIdleTimeout(ws);
			ws.handshake("127.0.0.1", "/");

			ws.text(true);
			beast::flat_buffer greeting;
			ws.read(greeting);

			const std::string hello
			    = json{ { "type", "hello" }, { "payload", json::object() } }.dump();
			ws.write(asio::buffer(hello));

			for (const auto& frame : _frames)
				ws.write(asio::buffer(frame));

			_sent.set_value();

			while (!_stop.load(std::memory_order_acquire))
				std::this_thread::sleep_for(std::chrono::milliseconds(10));

			detail::forceClose(ws);
		});

		if (_sent.get_future().wait_for(std::chrono::seconds(15)) != std::future_status::ready)
			throw std::runtime_error("RawFramesClient: timed out");
	}

	void stop()
	{
		_stop.store(true, std::memory_order_release);
		if (_run.joinable())
			_run.join();
	}

  private:
	uint16_t _port;
	std::vector<std::string> _frames;
	std::thread _run;
	std::promise<void> _sent;
	std::atomic<bool> _stop{ false };
};

// A raw WebSocket client that completes the hello handshake (reads the session's
// greeting and sends its own hello) and then waits for the next frame from the
// session. Returns the frame as text, or an empty string when nothing arrives
// within the window. Uses an explicit deadline timer instead of the stream idle
// timeout, so absence of a frame is detected deterministically and fast — unlike
// idle timeouts, which are not guaranteed to wake a blocked read promptly.
class RawPeersListReader {
  public:
	explicit RawPeersListReader(uint16_t port,
	                            std::chrono::milliseconds window = std::chrono::milliseconds(500))
	    : _port(port), _window(window)
	{}

	// Returns the next frame after the handshake, or "" if nothing arrived
	// within the window (i.e. the session announced nothing).
	std::string nextFrame()
	{
		std::promise<std::string> p;
		auto f = p.get_future();
		_run = std::thread([this, &p] {
			try
			{
				asio::io_context ioc;
				websocket::stream<beast::tcp_stream> ws(asio::make_strand(ioc));
				beast::get_lowest_layer(ws).expires_after(std::chrono::seconds(10));
				beast::get_lowest_layer(ws).connect(
				    tcp::endpoint(asio::ip::make_address("127.0.0.1"), _port));
				beast::get_lowest_layer(ws).expires_never();
				detail::applyIdleTimeout(ws);
				ws.handshake("127.0.0.1", "/");
				ws.text(true);

				beast::flat_buffer greeting;
				ws.read(greeting);

				const std::string hello
				    = json{ { "type", "hello" }, { "payload", json::object() } }.dump();
				ws.write(asio::buffer(hello));

				auto buf = std::make_shared<beast::flat_buffer>();
				asio::steady_timer deadline(asio::make_strand(ioc));
				deadline.expires_after(_window);
				std::atomic<bool> done{ false };

				deadline.async_wait([&](boost::system::error_code ec) {
					bool expected = false;
					if (ec || !done.compare_exchange_strong(expected, true))
						return;
					p.set_value("");
					boost::system::error_code ignore;
					ws.next_layer().socket().shutdown(tcp::socket::shutdown_both, ignore);
					ws.next_layer().socket().close(ignore);
				});
				ws.async_read(*buf, [&, buf](boost::system::error_code ec, std::size_t) {
					bool expected = false;
					if (!done.compare_exchange_strong(expected, true))
						return;
					deadline.cancel();
					p.set_value(ec ? std::string{} : beast::buffers_to_string(buf->data()));
				});

				ioc.run();
			}
			catch (const std::exception& e)
			{
				p.set_value("");
			}
		});

		std::string frame;
		if (f.wait_for(_window + std::chrono::seconds(5)) == std::future_status::ready)
			frame = f.get();
		if (_run.joinable())
			_run.join();
		return frame;
	}

  private:
	uint16_t _port;
	std::chrono::milliseconds _window;
	std::thread _run;
};

} // namespace

// --- PeerRegistry unit tests ------------------------------------------------

// A fresh registry tracks no peers in either set.
TEST(PeerRegistry, EmptyByDefault)
{
	PeerRegistry reg;
	EXPECT_EQ(reg.knownCount(), 0u);
	EXPECT_EQ(reg.connectedCount(), 0u);
	EXPECT_TRUE(reg.known().empty());
	EXPECT_TRUE(reg.connected().empty());
	EXPECT_FALSE(reg.isKnown("127.0.0.1:9000"));
	EXPECT_FALSE(reg.isConnected("127.0.0.1:9000"));
}

// A peer added as known is known but never connected.
TEST(PeerRegistry, AddKnownMarksKnownOnly)
{
	PeerRegistry reg;
	reg.addKnown("10.0.0.1:8000");
	EXPECT_TRUE(reg.isKnown("10.0.0.1:8000"));
	EXPECT_FALSE(reg.isConnected("10.0.0.1:8000"));
	EXPECT_EQ(reg.knownCount(), 1u);
	EXPECT_EQ(reg.connectedCount(), 0u);
}

// Marking a peer connected also makes it known.
TEST(PeerRegistry, AddConnectedMarksConnectedAndKnown)
{
	PeerRegistry reg;
	reg.addConnected("10.0.0.2:8000", nullptr);
	EXPECT_TRUE(reg.isKnown("10.0.0.2:8000"));
	EXPECT_TRUE(reg.isConnected("10.0.0.2:8000"));
	EXPECT_EQ(reg.knownCount(), 1u);
	EXPECT_EQ(reg.connectedCount(), 1u);
}

// Dropping a peer from the connected set keeps it known.
TEST(PeerRegistry, RemoveConnectedKeepsKnown)
{
	PeerRegistry reg;
	reg.addConnected("10.0.0.3:8000", nullptr);
	reg.removeConnected("10.0.0.3:8000");
	EXPECT_FALSE(reg.isConnected("10.0.0.3:8000"));
	EXPECT_TRUE(reg.isKnown("10.0.0.3:8000"));
	EXPECT_EQ(reg.connectedCount(), 0u);
	EXPECT_EQ(reg.knownCount(), 1u);
}

// Removing an address that was never connected is a safe no-op.
TEST(PeerRegistry, RemoveAbsentIsNoop)
{
	PeerRegistry reg;
	reg.removeConnected("10.0.0.4:8000");
	EXPECT_EQ(reg.knownCount(), 0u);
	EXPECT_EQ(reg.connectedCount(), 0u);
}

// Dropping one of two live sessions to the same address keeps the peer marked
// connected; only the last session's disconnect clears the flag.
TEST(PeerRegistry, DisconnectOneOfTwoKeepsOtherConnected)
{
	asio::io_context ioc;
	Node node;
	auto s1 = std::make_shared<NodeSession>(node, ioc);
	auto s2 = std::make_shared<NodeSession>(node, ioc);

	PeerRegistry reg;
	reg.addConnected("10.0.0.5:8000", s1);
	reg.addConnected("10.0.0.5:8000", s2);
	EXPECT_TRUE(reg.isConnected("10.0.0.5:8000"));
	EXPECT_EQ(reg.connectedCount(), 1u);

	reg.disconnect("10.0.0.5:8000", s1.get());
	EXPECT_TRUE(reg.isConnected("10.0.0.5:8000"))
	    << "the second live session must keep the peer connected";
	EXPECT_EQ(reg.connectedCount(), 1u);

	reg.disconnect("10.0.0.5:8000", s2.get());
	EXPECT_FALSE(reg.isConnected("10.0.0.5:8000"));
	EXPECT_EQ(reg.connectedCount(), 0u);
	EXPECT_TRUE(reg.isKnown("10.0.0.5:8000")) << "known set must survive disconnect";
}

// The known set is deduplicated and returned in sorted order.
TEST(PeerRegistry, KnownListDeduplicatesAndSorts)
{
	PeerRegistry reg;
	reg.addKnown("10.0.0.5:8000");
	reg.addKnown("10.0.0.5:8000");
	reg.addKnown("10.0.0.1:8000");
	EXPECT_EQ(reg.known(), std::vector<std::string>({ "10.0.0.1:8000", "10.0.0.5:8000" }));
	EXPECT_EQ(reg.knownCount(), 2u);
}

// Connected and known sets stay distinct: a peer known (not connected) does not
// appear in the connected list.
TEST(PeerRegistry, ConnectedListOnlyContainsConnected)
{
	PeerRegistry reg;
	reg.addKnown("10.0.0.6:8000");
	reg.addConnected("10.0.0.7:8000", nullptr);
	EXPECT_EQ(reg.connected(), std::vector<std::string>({ "10.0.0.7:8000" }));
	ASSERT_EQ(reg.known().size(), 2u);
	EXPECT_EQ(reg.knownCount(), 2u);
}

// --- Hello-handshake integration tests --------------------------------------

// Accept direction: on receiving a hello, the session registers the remote peer
// as both known and connected, and its own greeting says hello.
TEST(PeerHandshakeAccept, RegistersPeerOnHello)
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

	RawHelloClient client(port, {});
	RawHelloClient::Result res = client.waitHandshake();

	EXPECT_EQ(json::parse(res.greeting).at("type").get<std::string>(), "hello");
	EXPECT_TRUE(waitUntil([&] { return node.peers().isConnected(res.localAddr); }))
	    << "peer was never marked connected";
	EXPECT_TRUE(node.peers().isKnown(res.localAddr));

	// Tear the connection down and wait for the registry to release the session
	// before the node is destroyed.
	client.stop();
	EXPECT_TRUE(waitUntil([&] { return !node.peers().isConnected(res.localAddr); }));
	ioc.stop();
	worker.join();
}

// A node merges acquaintances announced by a peer, but never registers itself
// as a known peer.
TEST(PeerHandshakeAccept, MergesKnownPeersExcludingSelf)
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

	RawHelloClient client(port, { { "10.0.0.8:8000", false }, { selfAddr(), true } });
	RawHelloClient::Result res = client.waitHandshake();

	EXPECT_TRUE(waitUntil([&] { return node.peers().isKnown("10.0.0.8:8000"); }))
	    << "discovered peer was not merged from peersList";
	EXPECT_TRUE(node.peers().isKnown(res.localAddr));
	EXPECT_FALSE(node.peers().isKnown(selfAddr())) << "node must not register itself";

	client.stop();
	EXPECT_TRUE(waitUntil([&] { return !node.peers().isConnected(res.localAddr); }));
	ioc.stop();
	worker.join();
}

// On the accept side the peer is keyed by the address it announces in its hello
// (source IP from the socket + its listener port), not by the ephemeral source
// port of the dial. The session's own greeting carries the configured listen
// address so the peer can do the same.
TEST(PeerHandshakeAccept, KeysPeerByAnnouncedListenerAddress)
{
	auto port = freePort();
	asio::io_context ioc;
	Node node;
	node.setConfig(Config(Addr("0.0.0.0", "7000"), "."));

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

	RawHelloClient client(port, {}, 0, "127.0.0.1:9999");
	RawHelloClient::Result res = client.waitHandshake();

	EXPECT_EQ(json::parse(res.greeting).at("payload").at("addr").get<std::string>(),
	          "0.0.0.0:7000");

	EXPECT_TRUE(waitUntil([&] { return node.peers().isConnected("127.0.0.1:9999"); }))
	    << "peer was not keyed by its announced listener address";
	EXPECT_FALSE(node.peers().isConnected(res.localAddr))
	    << "ephemeral dial port must not leak into the registry";

	client.stop();
	ioc.stop();
	worker.join();
}

// Dial direction: the session dials a peer, and the peer's hello registers it.
TEST(PeerHandshakeDial, RegistersPeerAndMergesKnownPeers)
{
	auto port = freePort();
	asio::io_context ioc;
	Node node;

	RawHelloServer server(port, { { "10.0.0.9:8000", false } });
	server.start();
	server.waitBound();

	auto session = std::make_shared<NodeSession>(node, ioc);
	session->dial("127.0.0.1", std::to_string(port));

	std::thread worker([&] {
		asio::executor_work_guard<asio::io_context::executor_type> guard(ioc.get_executor());
		ioc.run();
	});

	server.waitHelloSent();
	std::string peer = "127.0.0.1:" + std::to_string(port);
	EXPECT_TRUE(waitUntil([&] { return node.peers().isConnected(peer); }))
	    << "dialed peer was never marked connected";
	EXPECT_TRUE(node.peers().isKnown(peer));
	EXPECT_TRUE(waitUntil([&] { return node.peers().isKnown("10.0.0.9:8000"); }))
	    << "discovered peer was not merged from peersList";

	server.stop();
	EXPECT_TRUE(waitUntil([&] { return !node.peers().isConnected(peer); }));
	ioc.stop();
	worker.join();
}

// When a peer disconnects, it is dropped from the connected set but stays known.
TEST(PeerHandshake, DisconnectDropsConnectedPeer)
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

	RawHelloClient client(port, {});
	RawHelloClient::Result res = client.waitHandshake();

	EXPECT_TRUE(waitUntil([&] { return node.peers().isConnected(res.localAddr); }))
	    << "peer never connected";

	client.stop();

	EXPECT_TRUE(waitUntil([&] { return !node.peers().isConnected(res.localAddr); }))
	    << "peer stayed connected after disconnect";
	EXPECT_TRUE(node.peers().isKnown(res.localAddr)) << "known set must survive disconnect";

	ioc.stop();
	worker.join();
}

// --- Hello-gate tests --------------------------------------------------------

// A valid-JSON frame whose type is not hello is rejected as the first frame.
TEST(HelloGate, RejectsNonHelloFirstFrame)
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

	std::string bad = json{ { "type", "status" }, { "payload", json::object() } }.dump();
	RawProbeClient client(port, std::move(bad));
	EXPECT_TRUE(client.run()) << "session should close on non-hello first frame";

	ioc.stop();
	worker.join();
}

// Non-JSON garbage as the first frame is rejected.
TEST(HelloGate, RejectsGarbageFirstFrame)
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

	RawProbeClient client(port, "this is not json {{{");
	EXPECT_TRUE(client.run()) << "session should close on non-JSON first frame";

	ioc.stop();
	worker.join();
}

// No hello within HELLO_TIMEOUT tears down the connection.
TEST(HelloGate, TimesOutWithoutHello)
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

	// No hello from the client — the session resends its greet until the retry
	// budget is exhausted, then tears the connection down.
	RawProbeClient client(port, "");
	EXPECT_TRUE(client.run()) << "session should close on hello timeout";

	ioc.stop();
	worker.join();
}

// While the peer has not replied with its hello, the session retransmits its
// greeting (more than the initial frame) before eventually giving up.
TEST(HelloRetry, RetransmitsGreeting)
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

	// Never reply with a hello; count how many greets arrive before the session
	// closes on the exhausted retry budget.
	RawProbeClient client(port, "");
	EXPECT_GE(client.hellosBeforeClose(), 2)
	    << "session should retransmit its hello greeting until the retry budget is spent";

	ioc.stop();
	worker.join();
}

// After the handshake is established, any further hello is a stale
// retransmission and must not tear the live connection down.
TEST(HelloGate, IgnoresStaleHello)
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

	// Send the primary hello, then another immediately after; the session should
	// stay connected through both.
	RawHelloClient client(port, {}, /*extraHellos=*/1);
	RawHelloClient::Result res = client.waitHandshake();

	EXPECT_TRUE(waitUntil([&] { return node.peers().isConnected(res.localAddr); }))
	    << "peer never connected";
	std::this_thread::sleep_for(std::chrono::milliseconds(300));
	EXPECT_TRUE(node.peers().isConnected(res.localAddr))
	    << "stale hello after handshake must not close the connection";

	client.stop();
	EXPECT_TRUE(waitUntil([&] { return !node.peers().isConnected(res.localAddr); }));
	ioc.stop();
	worker.join();
}

// A peersList entry marked connected (true) merges that peer into both the
// connected and known sets, keyed by ip:port.
TEST(PeersList, MarksConnectedViaMap)
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

	RawHelloClient client(port, { { "10.0.0.50:8000", true } });
	RawHelloClient::Result res = client.waitHandshake();

	EXPECT_TRUE(waitUntil([&] { return node.peers().isConnected("10.0.0.50:8000"); }))
	    << "connected peer from peersList was not marked connected";
	EXPECT_TRUE(node.peers().isKnown("10.0.0.50:8000"));
	EXPECT_TRUE(node.peers().isConnected(res.localAddr));

	client.stop();
	EXPECT_TRUE(waitUntil([&] { return !node.peers().isConnected(res.localAddr); }));
	ioc.stop();
	worker.join();
}

// A peersList frame sent before the hello initialization is a protocol
// violation: the connection must be initialized first.
TEST(PeersList, RejectedBeforeInit)
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

	std::string peersListFrame = json{
		{ "type", "peersList" },
		{ "payload", json{ { "peers", json{ { "10.0.0.60:8000", false } } } } }
	}.dump();
	RawProbeClient client(port, std::move(peersListFrame));
	EXPECT_TRUE(client.run()) << "session should reject a peersList frame before hello";

	ioc.stop();
	worker.join();
}

// peersList updates are merged at any time after initialization, not only at
// startup: a second update adds a peer announced later.
TEST(PeersList, AcceptedAnytimeMergesAllUpdates)
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

	RawPeersListClient client(port,
	                          { { { "10.0.0.70:8000", false } }, { { "10.0.0.71:8000", true } } });
	client.run();

	EXPECT_TRUE(waitUntil([&] { return node.peers().isKnown("10.0.0.70:8000"); }))
	    << "first peersList update was not merged";
	EXPECT_TRUE(waitUntil([&] { return node.peers().isConnected("10.0.0.71:8000"); }))
	    << "second later peersList update was not merged";

	client.stop();
	ioc.stop();
	worker.join();
}

// --- Outbound peersList tests -------------------------------------------------

// A peersList entry whose flag is not a boolean must be skipped, not fed to
// get<bool>() where it would throw out of the read handler and kill the
// io_context. The session survives and keeps processing valid frames.
TEST(PeersList, NonBooleanFlagDoesNotKillConnection)
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

	std::vector<std::string> frames{
		json{ { "type", "peersList" },
		      { "payload", json{ { "peers", { { "10.0.0.60:8000", "true" } } } } } }
		    .dump(),
		json{ { "type", "peersList" },
		      { "payload", json{ { "peers", { { "10.0.0.61:8000", true } } } } } }
		    .dump(),
	};

	RawFramesClient client(port, frames);
	client.run();

	EXPECT_TRUE(waitUntil([&] { return node.peers().isConnected("10.0.0.61:8000"); }))
	    << "session must survive a malformed peersList and merge the valid frame";
	EXPECT_FALSE(node.peers().isKnown("10.0.0.60:8000"))
	    << "non-boolean entry must be skipped, not registered";

	client.stop();
	ioc.stop();
	worker.join();
}

// The session announces its list exactly once, right after the hello handshake,
// merging the registry flags: connected peers as true and known-but-idle as
// false.
TEST(OutboundPeersList, AnnouncesOneshotAfterHandshake)
{
	auto port = freePort();
	asio::io_context ioc;
	Node node;
	node.peers().addConnected("10.0.0.55:8000", nullptr);
	node.peers().addKnown("10.0.0.56:8000");

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

	RawPeersListReader reader(port);
	std::string frame = reader.nextFrame();
	ASSERT_FALSE(frame.empty()) << "session never sent its peersList";

	json j = json::parse(frame);
	ASSERT_EQ(j.at("type").get<std::string>(), "peersList");
	const json peers = j.at("payload").at("peers");
	ASSERT_TRUE(peers.is_object());
	EXPECT_EQ(peers.at("10.0.0.55:8000").get<bool>(), true);
	EXPECT_EQ(peers.at("10.0.0.56:8000").get<bool>(), false);

	ioc.stop();
	worker.join();
}

// With no acquaintances a fresh session announces nothing on the first hello —
// the peersList is optional and only sent when there is something to say.
TEST(OutboundPeersList, SilentWhenNoOtherPeers)
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

	RawPeersListReader reader(port);
	EXPECT_TRUE(reader.nextFrame().empty()) << "session must not announce an empty peersList";

	ioc.stop();
	worker.join();
}
