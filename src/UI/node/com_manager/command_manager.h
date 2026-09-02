#pragma once
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "menu.h"

class CommandManager {
  public:
	CommandManager(NodeApi& api) : _api(api) {};
	void initCommands();
	bool execute(const std::string& name, std::vector<std::string> args);
	std::vector<CommandInfo> listCommands() const;

  private:
	template <typename T, typename... Args>
	void addCommand(const std::string& name, const std::string& desc, const std::string& usage,
	                Args&&...args);

	std::unordered_map<std::string, std::unique_ptr<MenuItem>> _commands;
	NodeApi& _api;
};
