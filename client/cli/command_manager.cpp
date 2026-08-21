#include "command_manager.h"

void CommandManager::initCommands(
    std::unordered_map<std::string, std::unique_ptr<MenuItem>>& _commands)
{
	auto add = [&](auto cmd) { _commands[cmd->name] = std::move(cmd); };
	add(std::make_unique<ExitCommand>(_client, "exit", "Exit the program", ""));
	add(std::make_unique<PrintCommand>(_client, "print", "Print the server path", ""));
	add(std::make_unique<MyPathCommand>(_client, "mypath", "Show the client path", ""));
	add(std::make_unique<ConnectCommand>(_client, "connect", "Connect to the server", ""));
	add(std::make_unique<ShowPathCommand>(_client, "showpath", "Show the server path", ""));
	add(std::make_unique<DownloadCommand>(_client, "download", "Download a file", ""));
	add(std::make_unique<SendFilesCommand>(_client, "sendfiles", "Send files", ""));
	add(std::make_unique<HelpCommand>(_client, _commands, "help", "Show help", "[command]"));
}