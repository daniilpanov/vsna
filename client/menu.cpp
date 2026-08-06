#include "menu.h"

void Menu::buildCommands() {
    auto add = [&](auto cmd) {
        commands[cmd->getName()] = std::move(cmd);
    };
    add(std::make_unique<HelpCommand>(commands));
    add(std::make_unique<ExitCommand>(isExit));
    add(std::make_unique<PrintCommand>(config));
    add(std::make_unique<MyPathCommand>(config));
    add(std::make_unique<ConnectCommand>(config));
    add(std::make_unique<ShowPathCommand>(config));
    add(std::make_unique<DownloadCommand>(config));
    add(std::make_unique<SendFilesCommand>(config));
}

void Menu::run() {
    std::string input;
    while (true) {
        std::cout << "> ";
        std::getline(std::cin, input);
        ARG_VECTOR args = splitArgs(input);

        if (args.empty()) continue;

        auto it = commands.find(args[0]);
        if (it == commands.end()) {
            std::cout << "Unknown command: " << args[0] << std::endl;
        } else {
            it->second->handle(ARG_VECTOR(args.begin() + 1, args.end()));
        }
        if (isExit) break;
    }
}

ARG_VECTOR splitArgs(STRING_ARG input) {
    ARG_VECTOR args;
    std::stringstream ss(input);
    std::string token;
    while (ss >> token) {
        args.push_back(token);
    }
    return args;
}

std::string toLowerCase(STRING_ARG str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(), ::tolower);
    return result;
}

void ExitCommand::handle(const ARG_VECTOR& args) {
    isExit = true;
    std::cout << "[~] Programm was exit." << std::endl;
}

void PrintCommand::handle(const ARG_VECTOR& args) {
    std::cout << config.toString() << std::endl;
}

void ConnectCommand::handle(const ARG_VECTOR& args) {
    if (args.empty()) {
        std::cout << "Usage: " << getUsage() << std::endl;
        return;
    }
}

void ShowPathCommand::handle(const ARG_VECTOR& args) {

}

void MyPathCommand::handle(const ARG_VECTOR& args) {

}

void SendFilesCommand::handle(const ARG_VECTOR& args) {
    if (args.empty()) {
        std::cout << "Usage: " << getUsage() << std::endl;
        return;
    }
}

void DownloadCommand::handle(const ARG_VECTOR& args) {
    if (args.empty()) {
        std::cout << "Usage: " << getUsage() << std::endl;
        return;
    }
}

void HelpCommand::handle(const ARG_VECTOR& args) {
    std::cout << "[=] Available commands:" << std::endl;
    for (const auto& [name, cmd] : commands) {
        auto usage = cmd->getUsage();
        std::cout << "\t" << name;
        if (usage[0] != '\0') std::cout << " " << usage;
        std::cout << " - " << cmd->getDescription() << std::endl;
    }
}