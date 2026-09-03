#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <memory>
#include <string>
#include <thread>

#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/system/error_code.hpp>

#include "node.h"

namespace asio = boost::asio;
namespace beast = boost::beast;
namespace websocket = boost::beast::websocket;
using tcp = boost::asio::ip::tcp;

namespace {

// Bind to an ephemeral port and return the chosen port number.
uint16_t freePort()
{
	asio::io_context ioc;
	tcp::acceptor acceptor(ioc, tcp::endpoint(asio::ip::make_address("127.0.0.1"), 0));
	return acceptor.local_endpoint().port();
}

// Poll a listening socket until it accepts a connection or we give up.
int waitForAccept(const std::string& host, uint16_t port, int attempts = 60)
{
	for (int i = 0; i < attempts; ++i)
	{
		boost::system::error_code ec;
		asio::io_context ioc;
		tcp::socket s(ioc);
		s.connect(tcp::endpoint(asio::ip::make_address(host), port), ec);
		s.close();
		if (!ec)
			return 0;
		std::this_thread::sleep_for(std::chrono::milliseconds(50));
	}
	return -1;
}

// A blocking WebSocket client: connects, sends `msg`, returns the echo reply.
std::string wsEchoClient(const std::string& host, uint16_t port, const std::string& msg)
{
	asio::io_context ioc;
	websocket::stream<beast::tcp_stream> ws(asio::make_strand(ioc));

	beast::get_lowest_layer(ws).expires_after(std::chrono::seconds(5));
	beast::get_lowest_layer(ws).connect(tcp::endpoint(asio::ip::make_address(host), port));
	beast::get_lowest_layer(ws).expires_never();
	ws.set_option(websocket::stream_base::timeout::suggested(beast::role_type::client));

	ws.handshake(host, "/");
	ws.write(asio::buffer(msg));

	beast::flat_buffer buffer;
	ws.read(buffer);
	std::string reply = beast::buffers_to_string(buffer.data());
	ws.close(websocket::close_code::normal);
	return reply;
}

// A WebSocket server that accepts one connection, writes `msg` to the peer, and
// then reads the peer's echo reply.  Returns the peer's reply.
std::string wsServerWritesThenReads(uint16_t port, const std::string& msg)
{
	asio::io_context ioc;
	tcp::acceptor acceptor(ioc, tcp::endpoint(asio::ip::make_address("127.0.0.1"), port));

	websocket::stream<beast::tcp_stream> ws(asio::make_strand(ioc));
	beast::get_lowest_layer(ws).expires_after(std::chrono::seconds(5));
	tcp::socket sock = acceptor.accept();
	ws.next_layer().socket() = std::move(sock);

	ws.set_option(websocket::stream_base::timeout::suggested(beast::role_type::server));
	ws.accept();

	ws.write(asio::buffer(msg));

	beast::flat_buffer buffer;
	ws.read(buffer);
	std::string reply = beast::buffers_to_string(buffer.data());
	ws.close(websocket::close_code::normal);
	return reply;
}

} // namespace

// Direction 1 (accept branch): a plain WebSocket client dials the symmetric Node;
// the Node accepts, echoes the client's message back.
TEST(NodeAccept, EchoesClientMessage)
{
	auto port = freePort();

	auto node = std::make_shared<Node>();
	node->setConfig(Config(Addr("127.0.0.1", std::to_string(port)), "/"));
	node->start();

	ASSERT_EQ(waitForAccept("127.0.0.1", port), 0)
	    << "Node listener did not come up on port " << port;

	EXPECT_EQ(wsEchoClient("127.0.0.1", port, "hello-from-client"), "hello-from-client");

	node->stop();
}

// Direction 2 (dial branch): a plain WebSocket server sends a message to the Node;
// the Node (dialing) echoes it back, which the server receives.
//
// NOTE: we must NOT poke the server port with a bare TCP probe first: the server's
// single acceptor.accept() would consume that probe instead of the Node's dial.
// The server is started, given a moment to listen, then the Node dials it.
TEST(NodeDial, EchoesServerMessage)
{
	auto serverPort = freePort();
	auto nodePort = freePort();

	const std::string sent = "hello-from-server";
	std::string serverReceivedReply;
	std::string serverError;

	std::thread serverThread([&] {
		try
		{
			serverReceivedReply = wsServerWritesThenReads(serverPort, sent);
		}
		catch (const std::exception& e)
		{
			serverError = e.what();
		}
	});

	// Give the server a moment to bind and start accepting.
	std::this_thread::sleep_for(std::chrono::milliseconds(200));

	auto node = std::make_shared<Node>();
	node->setConfig(Config(Addr("127.0.0.1", std::to_string(nodePort)), "/"));
	node->start();
	node->connect("127.0.0.1", std::to_string(serverPort));

	serverThread.join();

	EXPECT_TRUE(serverError.empty()) << "Server error: " << serverError;
	EXPECT_EQ(serverReceivedReply, sent);

	node->stop();
}
