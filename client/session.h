#include "pch.h"

class ClientSession : public std::enable_shared_from_this<ClientSession> {
  public:
	explicit ClientSession(asio::io_context& ioc)
	    : _resolver(asio::make_strand(ioc)), _ws(asio::make_strand(ioc))
	{}
	void run(char const *host, char const *port, char const *text);
	void on_resolve(beast::error_code ec, tcp::resolver::results_type results);
	void on_connect(beast::error_code ec, tcp::resolver::results_type::endpoint_type ep);
	void on_handshake(beast::error_code ec);
	void on_write(beast::error_code ec, std::size_t bytes_transferred);
	void on_read(beast::error_code ec, std::size_t bytes_transferred);
	void on_close(beast::error_code ec);

  private:
	tcp::resolver _resolver;
	websocket::stream<beast::tcp_stream> _ws;
	beast::flat_buffer _buffer;
	std::string _host;
	std::string _port;
	std::string _text;
};