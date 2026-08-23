#include "command_manager.h"


template <typename T, typename... Args>
void CommandManager::addCommand(STRING_ARG name, STRING_ARG desc, STRING_ARG usage, Args&&... args)
{
    _commands[name] = std::make_unique<T>(
        _client, CommandInfo{ name, desc, usage }, std::forward<Args>(args)...);
}

// Write commands in lower case!
void CommandManager::initCommands()
{
	addCommand<HelpCommand>("help", "Show help", "", *this);
	addCommand<ExitCommand>("exit", "Exit the program", "");
	addCommand<PrintCommand>("print", "Print the server path", "");
	addCommand<MyPathCommand>("mypath", "Show the client path", "");
	addCommand<ShowPathCommand>("show_path", "Show server path", "[path]");
	addCommand<ConnectCommand>("connect", "Connect to the server", "[ip:port]");
	addCommand<SendFilesCommand>("send_files", "Send file(s)", "[file | path]");
	addCommand<DownloadCommand>("download", "Download file(s)", "[file | path]");
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
