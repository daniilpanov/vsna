#include "menu.h"

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
    _isExit = true;
    std::cout << "[~] Programm was exit." << std::endl;
}

void PrintCommand::handle(const ARG_VECTOR& args) {
    std::cout << _config.toString() << std::endl;
}

void ConnectCommand::handle(const ARG_VECTOR& args) {
    if (args.empty()) {
        std::cout << "Usage: " << getUsage() << std::endl;
        return;
    }
    
    boost::asio::io_service io_service;
    tcp::resolver resolver(io_service);
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
    for (const auto& [name, cmd] : _commands) {
        auto usage = cmd->getUsage();
        std::cout << "\t" << name;
        if (usage[0] != '\0') std::cout << " " << usage;
        std::cout << " - " << cmd->getDescription() << std::endl;
    }
}