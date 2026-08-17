#include "command_manager.h"


void CommandManager::init_commands(std::unordered_map<std::string, std::unique_ptr<MenuItem>>& _commands) {
    auto add = [&](auto cmd) {
        _commands[std::string(cmd->getName())] = std::move(cmd);
    };
    add(std::make_unique<HelpCommand>(_client, _commands));
    add(std::make_unique<ExitCommand>(_client));
    add(std::make_unique<PrintCommand>(_client));
    add(std::make_unique<MyPathCommand>(_client));
    add(std::make_unique<ConnectCommand>(_client));
    add(std::make_unique<ShowPathCommand>(_client));
    add(std::make_unique<DownloadCommand>(_client));
    add(std::make_unique<SendFilesCommand>(_client));
}