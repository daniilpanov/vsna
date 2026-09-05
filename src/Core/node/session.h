#pragma once
#include <atomic>
#include <chrono>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

#include "message.h"
#include "pch.h"

class Node;

// A persistent symmetric peer session. A single connection is first initialized
// with a hello frame, then stays alive across many messages. Incoming frames are
// routed to the handler registered for their message *type* (or to a default
// handler when none is registered); the envelope carries no transaction id, so
// a frame is identified solely by its type. Writes are queued so send() is safe
// to call from any thread.
class NodeSession : public std::enable_shared_from_this<NodeSession> {
  public:
	using Handler = std::function<void(const Message&)>;

	// Interval between hello handshake retransmissions.
	static constexpr auto HELLO_RETRY{ std::chrono::milliseconds(VSNA_HELLO_RETRY_MS) };

	// Number of retransmissions. The greeting is sent once and then resent up to
	// HELLO_RETRIES more times (every HELLO_RETRY) until the peer's first reply
	// hello arrives. The same count bounds how long we wait for that reply hello
	// before tearing the connection down.
	static constexpr int HELLO_RETRIES{ VSNA_HELLO_RETRIES };

	// Keepalive: a ping is sent every PING_INTERVAL only when no other (non-ping)
	// frame has been written since the previous tick.
	static constexpr auto PING_INTERVAL{ std::chrono::milliseconds(VSNA_PING_INTERVAL_MS) };

	// Idle timeout: the connection is torn down if no frame arrives from the peer
	// within IDLE_TIMEOUT. Any incoming frame (including a ping) refreshes it.
	static constexpr auto IDLE_TIMEOUT{ std::chrono::milliseconds(VSNA_IDLE_TIMEOUT_MS) };

	explicit NodeSession(Node& node, boost::asio::io_context& ioc);

	// Server direction: a socket accepted by the acceptor.
	void accept(tcp::socket socket);

	// Client direction: dial a remote peer.
	void dial(const std::string& host, const std::string& port);

	// Deterministically close the connection and release the peer registry and
	// node references. Called by the node during shutdown; idempotent.
	void shutdown();

	// Enqueue a message to be serialized and written on this connection.
	void send(const Message& msg);

	// Register a handler for a specific message type. Incoming frames of that
	// type are delivered to it (after the hello/peersList control frames are
	// processed internally).
	void onType(MessageType type, Handler handler);

	// Register a default handler for message types with no registered handler.
	void onMessage(Handler handler);

  private:
	Node& _node;
	boost::asio::io_context& _ioc;
	tcp::resolver _resolver;
	websocket::stream<beast::tcp_stream> _ws;
	beast::flat_buffer _buffer;
	std::string _host;
	std::string _port;
	std::string _remote;

	asio::steady_timer _hello_timer;
	asio::steady_timer _hello_retry_timer;
	asio::steady_timer _ping_timer;
	asio::steady_timer _idle_timer;
	int _hello_sends{ 0 };
	std::atomic<bool> _initialized{ false };
	std::atomic<bool> _closing{ false };
	// True when a non-ping frame has been written since the last ping tick,
	// which suppresses the next keepalive ping.
	bool _ping_suppressed{ false };

	std::mutex _write_mutex;
	std::deque<Message> _write_queue;
	bool _writing{ false };
	std::string _outgoing;

	std::unordered_map<MessageType, Handler> _type_handlers;
	Handler _default_handler;

	void setup_hello();
	void setup_hello_timer();
	void send_hello();
	void retry_hello(beast::error_code ec);
	void on_hello(const Message& msg);
	void on_peers_list(const Message& msg);
	void send_peers_list();
	void setup_keepalive();
	void schedule_ping();
	void send_ping();
	void retry_ping(beast::error_code ec);
	void refresh_idle();
	static bool isValidHello(const Message& msg);
	void close(const char *reason);
	void do_read();
	void do_write_next();
	void route(const Message& msg);
	void on_accept(beast::error_code ec);
	void on_resolve(beast::error_code ec, tcp::resolver::results_type results);
	void on_connect(beast::error_code ec, tcp::resolver::results_type::endpoint_type ep);
	void on_handshake(beast::error_code ec);
	void on_read(beast::error_code ec, std::size_t bytes_transferred);
	void on_write(beast::error_code ec, std::size_t bytes_transferred);
};
