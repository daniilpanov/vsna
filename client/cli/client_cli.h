#pragma once
#include <boost/asio.hpp>
#include <iostream>
#include <libs/CLI11.hpp>
#include <memory>
#include <string>
#include <unordered_map>

#include "client.h"
#include "command_manager.h"
#include "config.h"
#include "menu.h"
#include "utils.h"

class ClientCLI {
	Client& _client;
	std::unordered_map<std::string, std::unique_ptr<MenuItem>> _commands;

  public:
	ClientCLI(Client& client) : _client(client)
	{}
	void CLIParse(int argc, char **argv);
	void run(int argc, char **argv);
};
