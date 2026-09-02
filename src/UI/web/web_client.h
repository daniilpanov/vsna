#pragma once
#include <atomic>
#include <string>

#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/core.hpp>

#include "ui_controller.h"

namespace beast = boost::beast;
namespace http = boost::beast::http;
namespace asio = boost::asio;
using tcp = boost::asio::ip::tcp;

using Request = boost::beast::http::request<boost::beast::http::string_body>;
using Response = boost::beast::http::response<boost::beast::http::string_body>;

// Isolated web front-end (vsna_web). Owns a UiController (and so a NodeApi)
// plus its own Boost.Beast HTTP server. It exposes the NodeApi over a small
// REST/JSON API and serves static HTML/JS templates. It never touches the
// Core directly — everything goes through the NodeApi via UiController.
class WebClient {
  public:
	WebClient() : _ui()
	{}

	// Parse options, start the node, print a banner and serve HTTP until
	// interrupted. Returns the process exit code.
	int run(int argc, char **argv);

  private:
	UiController _ui;
	std::string _httpIp{ "0.0.0.0" };
	std::string _httpPort{ "8080" };
	std::string _templates; // directory containing the templates to serve
	std::atomic_bool _stop{ false };

	void parse(int argc, char **argv);
	void serve(); // blocking accept loop
	void handle(tcp::socket socket);
	Response route(const Request& req);
	Response handleApi(const Request& req);
	Response serveFile(const std::string& name, const std::string& contentType);
};

// Build an HTTP response with a given status and content-type.
boost::beast::http::response<boost::beast::http::string_body>
make_response(boost::beast::http::status status, const std::string& contentType,
              const std::string& body);