#pragma once
#include <boost/asio.hpp>
#include <iostream>
#include <CLI/CLI.hpp>
#include <memory>
#include <string>
#include <unordered_map>

#include "client.h"
#include "command_manager.h"
#include "config.h"
#include "menu.h"
#include "helper.h"

class ClientUI {
	Client _client;
	CommandManager _commandManager;

  public:
	ClientUI() : _commandManager(_client)
	{}
	void CLIParse(int argc, char **argv);
	void run(int argc, char **argv);
	std::pair<std::string, std::vector<std::string> > parseArgs(STRING_ARG input);
};
