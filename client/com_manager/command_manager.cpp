#include "command_manager.h"


template <typename T>
void CommandManager::addCommand(STRING_ARG name, STRING_ARG desc, STRING_ARG usage)
{
	_commands[name] = std::make_unique<T>(_client, CommandInfo{ name, desc, usage });
}

// Write commands in lower case!
void CommandManager::initCommands()
{
	addCommand<ExitCommand>("exit", "Exit the program", "");
	addCommand<PrintCommand>("print", "Print the server path", "");
	addCommand<MyPathCommand>("mypath", "Show the client path", "");
	addCommand<SendFilesCommand>("send_files", "Send files", "[file | path]");
	addCommand<DownloadCommand>("download", "Download a file", "[file | path]");
	addCommand<ConnectCommand>("connect", "Connect to the server", "[ip:port]");
	addCommand<ShowPathCommand>("show_path", "Show the server path", "[file | path]");
	_commands["help"] = std::make_unique<HelpCommand>(
		_client, *this, CommandInfo{ "help", "Show help", "" });
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
		result.push_back(cmd->_info);
	return result;
}
