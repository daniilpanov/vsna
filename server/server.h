#include <iostream>
#include <thread>
#include <boost/beast/websocket.hpp>
#include <boost/beast/core.hpp>
#include "config.h"

using tcp = boost::asio::ip::tcp;

void start_server(const Config&);