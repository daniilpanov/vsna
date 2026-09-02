#pragma once
#include <boost/asio.hpp>
#include <CLI/CLI.hpp>
#include <iostream>
#include <string>
#include <vector>

#include "command_manager.h"
#include "helper.h"
#include "node_api.h"

class NodeUI {
	NodeApi _api;
	CommandManager _commandManager;

  public:
	NodeUI() : _commandManager(_api)
	{}
	std::vector<std::string> CLIParse(int argc, char **argv);
	void run(int argc, char **argv);
	void repl();
	std::pair<std::string, std::vector<std::string>> parseArgs(const std::string& input);
};