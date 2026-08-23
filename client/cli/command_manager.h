#pragma once
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "types.h"
#include "menu.h"

struct CommandInfo {
	std::string name;
	std::string description;
	std::string usage;
};

class CommandManager {
  public:
	CommandManager(Client& client) : _client(client) {};
	void initCommands();
	bool execute(STRING_ARG name, ARG_VECTOR args);
	std::vector<CommandInfo> listCommands() const;

  private:
	template <typename T>
	void addCommand(STRING_ARG name, STRING_ARG desc, STRING_ARG usage);

	using COMMAND_MAP = std::unordered_map<std::string, std::unique_ptr<MenuItem>>;
	COMMAND_MAP _commands;
	Client& _client;
};
