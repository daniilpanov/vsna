#pragma once
#include <memory>
#include <unordered_map>

#include "menu.h"

class CommandManager {
  public:
	CommandManager(Client& client) : _client(client) {};
	void initCommands(std::unordered_map<std::string, std::unique_ptr<MenuItem>>& _commands);

  private:
	Client& _client;
};