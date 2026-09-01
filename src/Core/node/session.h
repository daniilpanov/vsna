#pragma once
#include <cstdint>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

#include "message.h"
#include "pch.h"

class Node;
class TransactionManager;

// A persistent symmetric peer session. A single connection stays alive across
// many messages and multiplexes several transactions at once, distinguished by
// their tx_id: incoming messages are routed to the handler registered for that
// tx_id (or to a default handler when none is registered). Writes are queued so
// send() is safe to call from any thread.
class NodeSession : public std::enable_shared_from_this<NodeSession> {
  public:
	using Handler = std::function<void(const Message&)>;

	// Reserved id for control messages (e.g. hello) not tied to a transaction.
	static constexpr uint64_t HELLO_TX{ 0 };

	explicit NodeSession(Node& node, boost::asio::io_context& ioc);

	// Server direction: a socket accepted by the acceptor.
	void accept(tcp::socket socket);

	// Client direction: dial a remote peer.
	void dial(const std::string& host, const std::string& port);

	// Enqueue a message to be serialized and written on this connection.
	void send(const Message& msg);

	// Register a handler for a specific transaction id. Messages carrying that
	// tx_id are delivered to it.
	void onTx(uint64_t tx_id, Handler handler);

	// Register a default handler for messages whose tx_id has no handler.
	void onMessage(Handler handler);

	// Transaction manager for this connection (created lazily). Owns the
	// receive-side claim handler and drives outgoing transfers.
	std::shared_ptr<TransactionManager> txn();

	// The strand serializing all work on this connection.
	boost::asio::any_io_executor strand()
	{
		return _ws.get_executor();
	}

  private:
	Node& _node;
	boost::asio::io_context& _ioc;
	tcp::resolver _resolver;
	websocket::stream<beast::tcp_stream> _ws;
	beast::flat_buffer _buffer;
	std::string _host;
	std::string _port;
	std::string _remote;

	std::mutex _write_mutex;
	std::deque<Message> _write_queue;
	bool _writing{ false };
	std::string _outgoing;

	std::unordered_map<uint64_t, Handler> _handlers;
	Handler _default_handler;

	std::shared_ptr<TransactionManager> _txn;

	void setup_hello();
	void send_hello();
	void on_hello(const Message& msg);
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
