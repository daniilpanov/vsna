#include "command_manager.h"

// Write commands in lower case!
void CommandManager::initCommands()
{
	addCommand<ExitCommand>("exit", "Exit the program", "");
	addCommand<SendFilesCommand>("send_files", "Send files", "[file | path]");
	addCommand<DownloadCommand>("download", "Download a file", "[file | path]");
	addCommand<PrintCommand>("print", "Print the server path", "");
	addCommand<MyPathCommand>("mypath", "Show the client path", "");
	addCommand<ConnectCommand>("connect", "Connect to the server", "[ip:port]");
	addCommand<ShowPathCommand>("show_path", "Show the server path", "[file | path]");
	_commands["help"] = std::make_unique<HelpCommand>(
		_client, *this, "help", "Show help", "");
}

bool CommandManager::execute(STRING_ARG name, ARG_VECTOR args)
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
		result.push_back({std::string(name), std::string(cmd->desc), std::string(cmd->usage)});
	return result;
}

template <typename T>
void CommandManager::addCommand(STRING_ARG name, STRING_ARG desc, STRING_ARG usage)
{
	_commands[std::string(name)] = std::make_unique<T>(_client, name, desc, usage);
}
