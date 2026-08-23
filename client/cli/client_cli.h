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
	CommandManager _commandManager;

  public:
	ClientCLI(Client& client) : _client(client), _commandManager(client)
	{}
	void CLIParse(int argc, char **argv);
	void run(int argc, char **argv);
};
