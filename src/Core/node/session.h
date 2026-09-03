#pragma once
#include <memory>
#include <string>

#include "pch.h"

class Node;

// A symmetric peer session. The same class handles both directions of the
// connection: when created from an accepted socket it performs the server
// handshake, when created through dial() it performs the client handshake.
// After the handshake both roles run the exact same read/write loop.
class NodeSession : public std::enable_shared_from_this<NodeSession> {
  public:
	explicit NodeSession(Node& node, boost::asio::io_context& ioc);

	// Server direction: a socket accepted by the acceptor.
	void accept(tcp::socket socket);

	// Client direction: dial a remote peer.
	void dial(const std::string& host, const std::string& port);

  private:
	Node& _node;
	boost::asio::io_context& _ioc;
	tcp::resolver _resolver;
	websocket::stream<beast::tcp_stream> _ws;
	beast::flat_buffer _buffer;
	std::string _host;
	std::string _port;

	void do_read();
	void on_accept(beast::error_code ec);
	void on_resolve(beast::error_code ec, tcp::resolver::results_type results);
	void on_connect(beast::error_code ec, tcp::resolver::results_type::endpoint_type ep);
	void on_handshake(beast::error_code ec);
	void on_read(beast::error_code ec, std::size_t bytes_transferred);
	void on_write(beast::error_code ec, std::size_t bytes_transferred);
};
