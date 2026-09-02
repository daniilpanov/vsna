#include "command_manager.h"

template <typename T, typename... Args>
void CommandManager::addCommand(const std::string& name, const std::string& desc,
                                const std::string& usage, Args&&...args)
{
	_commands[name]
	    = std::make_unique<T>(_api, CommandInfo{ name, desc, usage }, std::forward<Args>(args)...);
}

// Write commands in lower case!
void CommandManager::initCommands()
{
	addCommand<HelpCommand>("help", "Show help", "", *this);
	addCommand<ExitCommand>("exit", "Exit the program", "");
	addCommand<PrintCommand>("print", "Print the node config", "");
	addCommand<MyPathCommand>("mypath", "Show the node path", "");
	addCommand<ConnectCommand>("connect", "Connect to a peer", "[ip:port]");
	addCommand<AddCommand>("add", "Add a peer manually", "[ip:port]");
	addCommand<PeersCommand>("peers", "List known/connected peers", "");
	addCommand<ConnectAllCommand>("connect_all", "Connect to all known peers", "");
	addCommand<SendCommand>("send", "Send a file via transactional transfer",
	                        "[peer] [path] or [path]");
}

bool CommandManager::execute(const std::string& name, std::vector<std::string> args)
{
	auto it = _commands.find(std::string(name));
	if (it == _commands.end())
	{
		std::cout << "[!] Unknown command: " << name << std::endl;
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
