#pragma once
#include <boost/asio.hpp>
#include <iostream>
#include <CLI/CLI.hpp>
#include <memory>
#include <string>

#include "config.h"
#include "server.h"
#include "helper.h"

class ServerCLI {
  public:
	ServerCLI() = default;
	void run(int argc, char **argv);
	void CLIParse(int argc, char **argv);

  private:
	std::shared_ptr<Server> _server{ std::make_shared<Server>() };
};