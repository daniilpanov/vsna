#include "command_manager.h"

template <typename T, typename... Args>
void CommandManager::addCommand(const std::string& name, const std::string& desc,
                                const std::string& usage, Args&&...args)
{
	_commands[name] = std::make_unique<T>(_client, CommandInfo{ name, desc, usage },
	                                      std::forward<Args>(args)...);
}

// Write commands in lower case!
void CommandManager::initCommands()
{
	addCommand<HelpCommand>("help", "Show help", "", *this);
	addCommand<ExitCommand>("exit", "Exit the program", "");
	addCommand<PrintCommand>("print", "Print the server path", "");
	addCommand<MyPathCommand>("mypath", "Show the client path", "");
	addCommand<ConnectCommand>("connect", "Connect to the server", "[ip:port]");
}

bool CommandManager::execute(const std::string& name, std::vector<std::string> args)
{
	auto it = _commands.find(std::string(name));
	if (it == _commands.end())
	{
		std::cout << "Unknown command: " << name << std::endl;
		return false;
	}
	return it->second->handle(args);
}

std::vector<CommandInfo> CommandManager::listCommands() const
{
	std::vector<CommandInfo> result;
	result.reserve(_commands.size());
	for (const auto& [name, cmd] : _commands)
		result.push_back(cmd->_info);
	return result;
}
