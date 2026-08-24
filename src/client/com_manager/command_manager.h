#pragma once
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "types.h"
#include "menu.h"

class CommandManager {
  public:
	CommandManager(Client& client) : _client(client) {};
	void initCommands();
	bool execute(STRING_ARG name, STRING_VECTOR args);
	std::vector<CommandInfo> listCommands() const;

  private:
    template <typename T, typename... Args>
    void addCommand(STRING_ARG name, STRING_ARG desc, STRING_ARG usage, Args&&... args);

	std::unordered_map<std::string, std::unique_ptr<MenuItem>> _commands;
	Client& _client;
};
