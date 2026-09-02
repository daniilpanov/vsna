#include "web_client.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

#include <CLI/CLI.hpp>
#include <nlohmann/json.hpp>

#include "helper.h"

Response make_response(http::status status, const std::string& contentType, const std::string& body)
{
	Response res{ status, 11 };
	res.set(http::field::server, "vsna_web");
	res.set(http::field::content_type, contentType);
	res.body() = body;
	res.prepare_payload();
	return res;
}

int WebClient::run(int argc, char **argv)
{
	parse(argc, argv);

	_ui.start();

	std::cout << "[~] Serving on http://" << _httpIp << ":" << _httpPort << std::endl;
	std::cout << _ui.describe() << std::endl;

	serve();
	return EXIT_SUCCESS;
}

void WebClient::parse(int argc, char **argv)
{
	CLI::App app{ "VSNA Web Client" };
	app.allow_extras();

	std::string nodeIp{ "0.0.0.0" };
	std::string nodePort{ "5555" };
	std::string nodePath{ "/" };

	app.add_option("--http-ip", _httpIp, "Address to bind the HTTP server on");
	app.add_option("--http-port", _httpPort, "Port of the HTTP server");
	app.add_option("--templates", _templates, "Directory with web templates to serve");
	app.add_option("-i,--ip", nodeIp, "Node listener IP");
	app.add_option("-p,--port", nodePort, "Node listener port");
	app.add_option("-d,--dir", nodePath, "Node path to store and serve files");

	try
	{
		app.parse(argc, argv);
	}
	catch (const CLI::ParseError& e)
	{
		app.exit(e);
		if (dynamic_cast<const CLI::Success *>(&e) == nullptr)
			std::cerr << e.what() << std::endl;
		exit(dynamic_cast<const CLI::Success *>(&e) != nullptr ? EXIT_SUCCESS : EXIT_FAILURE);
	}

	_ui.api().configure(nodeIp, nodePort, nodePath);
}

void WebClient::serve()
{
	try
	{
		asio::io_context ioc{ 1 };
		tcp::acceptor acceptor{ ioc,
			                    { asio::ip::make_address(_httpIp),
			                      static_cast<unsigned short>(std::stoi(_httpPort)) } };

		while (!_stop)
		{
			tcp::socket socket{ ioc };
			acceptor.accept(socket);
			handle(std::move(socket));
		}
	}
	catch (const std::exception& e)
	{
		std::cerr << "[!] HTTP server error: " << e.what() << std::endl;
	}
}

void WebClient::handle(tcp::socket socket)
{
	try
	{
		beast::flat_buffer buffer;
		Request req;
		http::read(socket, buffer, req);

		Response res = route(req);
		http::write(socket, res);
	}
	catch (const std::exception& e)
	{
		std::cerr << "[!] Request handling failed: " << e.what() << std::endl;
	}
	boost::system::error_code ec;
	socket.shutdown(tcp::socket::shutdown_send, ec);
}

Response WebClient::route(const Request& req)
{
	// Static templates first.
	if (req.method() == http::verb::get)
	{
		std::string target = std::string(req.target());
		if (target == "/" || target == "/index.html")
			return serveFile("index.html", "text/html");
		if (target == "/app.js")
			return serveFile("app.js", "application/javascript");
		if (target == "/style.css")
			return serveFile("style.css", "text/css");
	}

	// REST/JSON API.
	std::string target = std::string(req.target());
	if (target.rfind("/api/", 0) == 0)
		return handleApi(req);

	return make_response(http::status::not_found, "text/plain", "Not found");
}

Response WebClient::handleApi(const Request& req)
{
	using json = nlohmann::json;
	std::string path = std::string(req.target());
	std::string method = std::string(http::to_string(req.method()));

	auto badRequest = [&](const std::string& msg) {
		return make_response(http::status::bad_request, "application/json",
		                     json{ { "error", msg } }.dump());
	};
	auto notFound = [&]() {
		return make_response(http::status::not_found, "application/json",
		                     json{ { "error", "Unknown endpoint: " + path } }.dump());
	};

	// GET /api/describe
	if (path == "/api/describe" && method == "GET")
		return make_response(
		    http::status::ok, "application/json",
		    json{ { "addr", _ui.api().addr() }, { "path", _ui.api().path() } }.dump());

	// GET /api/peers
	if (path == "/api/peers" && method == "GET")
		return make_response(
		    http::status::ok, "application/json",
		    json{ { "known", _ui.api().knownPeers() }, { "connected", _ui.api().connectedPeers() } }
		        .dump());

	// Parse any JSON body needed by the POST endpoints below.
	json body;
	if (!req.body().empty())
	{
		try
		{
			body = json::parse(req.body());
		}
		catch (const std::exception&)
		{
			return badRequest("Invalid JSON body");
		}
	}

	auto hostPort = [&](const json& j) -> std::pair<std::string, std::string> {
		if (j.contains("host") && j.contains("port"))
			return { j.at("host").get<std::string>(), j.at("port").get<std::string>() };
		auto addr = j.value("addr", "");
		auto parts = split(addr, ":");
		if (parts.size() == 2)
			return { parts[0], parts[1] };
		return {};
	};

	// POST /api/connect
	if (path == "/api/connect" && method == "POST")
	{
		auto [host, port] = hostPort(body);
		if (host.empty() || port.empty())
			return badRequest("Expected { host, port } or { addr: 'host:port' }");
		_ui.api().connect(host, port);
		return make_response(http::status::ok, "application/json", json{ { "ok", true } }.dump());
	}

	// POST /api/add
	if (path == "/api/add" && method == "POST")
	{
		auto [host, port] = hostPort(body);
		if (host.empty() || port.empty())
			return badRequest("Expected { host, port } or { addr: 'host:port' }");
		_ui.api().addPeer(host, port);
		return make_response(http::status::ok, "application/json", json{ { "ok", true } }.dump());
	}

	// POST /api/connect_all
	if (path == "/api/connect_all" && method == "POST")
	{
		_ui.api().connectToAllKnown();
		return make_response(http::status::ok, "application/json", json{ { "ok", true } }.dump());
	}

	// POST /api/send
	if (path == "/api/send" && method == "POST")
	{
		std::string filePath = body.value("path", "");
		if (filePath.empty())
			return badRequest("Expected { path }");
		_ui.api().sendFile(body.value("peer", ""), filePath);
		return make_response(http::status::accepted, "application/json",
		                     json{ { "accepted", true } }.dump());
	}

	return notFound();
}

Response WebClient::serveFile(const std::string& name, const std::string& contentType)
{
	std::filesystem::path base = _templates.empty() ? "templates" : _templates;
	std::ifstream in(base / name, std::ios::binary);
	if (!in)
		return make_response(http::status::not_found, "text/plain", "Missing template: " + name);
	std::ostringstream ss;
	ss << in.rdbuf();
	return make_response(http::status::ok, contentType, ss.str());
}