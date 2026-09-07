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

// A persistent symmetric peer session. A single connection stays alive across
// many messages. Incoming frames are routed to the handler registered for their
// message *type* (or to a default handler when none is registered). Writes are
// queued so send() is safe to call from any thread.
class NodeSession : public std::enable_shared_from_this<NodeSession> {
  public:
	using Handler = std::function<void(const Message&)>;

	explicit NodeSession(Node& node, boost::asio::io_context& ioc);

	// Server direction: a socket accepted by the acceptor.
	void accept(tcp::socket socket);

	// Client direction: dial a remote peer.
	void dial(const std::string& host, const std::string& port);

	// Enqueue a message to be serialized and written on this connection.
	void send(const Message& msg);

	// Register a handler for a specific message type. Incoming frames of that
	// type are delivered to it.
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

	std::mutex _write_mutex;
	std::deque<Message> _write_queue;
	bool _writing{ false };
	std::string _outgoing;

	std::unordered_map<MessageType, Handler> _type_handlers;
	Handler _default_handler;

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
