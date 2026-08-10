#include "menu.h"


void ExitCommand::handle(CONST_ARG_VECTOR args) {
    _isExit = true;
    std::cout << "[~] Programm was exit." << std::endl;
}

void HelpCommand::handle(CONST_ARG_VECTOR args) {
    std::cout << "[=] Available commands:" << std::endl;
    for (const auto& [name, cmd] : _commands) {
        auto usage = cmd->getUsage();
        std::cout << "\t" << name;
        if (!usage.empty()) std::cout << " " << usage;
        std::cout << " - " << cmd->getDescription() << std::endl;
    }
}