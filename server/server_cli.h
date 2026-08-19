#pragma once
#include <boost/asio.hpp>
#include <iostream>
#include <libs/CLI11.hpp>
#include <memory>
#include <string>

#include "config.h"
#include "server.h"
#include "utils.h"

class ServerCLI {
	std::shared_ptr<Server> _server;

  public:
	ServerCLI() : _server(std::make_shared<Server>())
	{}
	void run(int argc, char **argv);
	void CLIParse(int argc, char **argv);
};