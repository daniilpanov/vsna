#include "pch.h"

class ServerSession : public std::enable_shared_from_this<ServerSession> {
  public:
	explicit ServerSession(tcp::socket socket) : _ws(std::move(socket))
	{}
	void run();

  private:
	websocket::stream<tcp::socket> _ws;
	beast::flat_buffer _buffer;

	void on_accept(beast::error_code ec);
	void do_read();
	void on_read(beast::error_code ec, std::size_t bytes_transferred);
	void on_write(beast::error_code ec, std::size_t bytes_transferred);
};
