#pragma once
#include <boost/asio.hpp>
#include <CLI/CLI.hpp>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "command_manager.h"
#include "config.h"
#include "node.h"

class NodeUI {
	std::shared_ptr<Node> _node{ std::make_shared<Node>() };
	CommandManager _commandManager;

  public:
	NodeUI() : _commandManager(*_node)
	{}
	std::vector<std::string> CLIParse(int argc, char **argv);
	void run(int argc, char **argv);
	void repl();
	std::pair<std::string, std::vector<std::string>> parseArgs(const std::string& input);
};
