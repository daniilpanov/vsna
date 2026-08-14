#pragma once
#include <unordered_map>
#include <memory>
#include <string>
#include <string_view>
#include <vector>
#include "config.h"
#include "client.h"
#include "types.h"

class MenuItem {
protected:
    std::shared_ptr<Client> _client;
public:
    virtual ~MenuItem() = default;
    // TODO: Is neccessary to init _client by std::make_shared<Client>?
    explicit MenuItem(std::shared_ptr<Client> client) : _client(client) {}
    virtual void handle(CONST_ARG_VECTOR) = 0;
    virtual std::string_view getName() const = 0;
    virtual std::string_view getDescription() const = 0;
    virtual std::string_view getUsage() const { return {}; }
};


class ConnectCommand : public MenuItem {
public:
    explicit ConnectCommand(std::shared_ptr<Client> client) : MenuItem(client) {}
    void handle(CONST_ARG_VECTOR args) override { _client->connect(args); };
    std::string_view getName() const override { return "connect"; }
    std::string_view getDescription() const override { return "Connect to the server"; }
    std::string_view getUsage() const override { return "[ip:port]"; }
};


class ShowPathCommand : public MenuItem {
public:
    explicit ShowPathCommand(std::shared_ptr<Client> client) : MenuItem(client) {}
    void handle(CONST_ARG_VECTOR args) override { _client->showPath(args); };
    std::string_view getName() const override { return "path"; }
    std::string_view getDescription() const override { return "Show the server path"; }
    std::string_view getUsage() const override { return "[name]"; }
};


class MyPathCommand : public MenuItem {
public:
    explicit MyPathCommand(std::shared_ptr<Client> client) : MenuItem(client) {}
    void handle(CONST_ARG_VECTOR args) override { _client->myPath(args); };
    std::string_view getName() const override { return "mypath"; }
    std::string_view getDescription() const override { return "Show the client path"; }
    std::string_view getUsage() const override { return "[name]"; }
};


class SendFilesCommand : public MenuItem {
public:
    explicit SendFilesCommand(std::shared_ptr<Client> client) : MenuItem(client) {}
    void handle(CONST_ARG_VECTOR args) override { _client->sendFiles(args); };
    std::string_view getName() const override { return "send"; }
    std::string_view getDescription() const override { return "Send files to the server"; }
    std::string_view getUsage() const override { return "<file1 | path1> [file2] ..."; }
};


class DownloadCommand : public MenuItem {
public:
    explicit DownloadCommand(std::shared_ptr<Client> client) : MenuItem(client) {}
    void handle(CONST_ARG_VECTOR args) override { _client->download(args); };
    std::string_view getName() const override { return "download"; }
    std::string_view getDescription() const override { return "Download files from the server"; }
    std::string_view getUsage() const override { return "<file1 | path1> [file2] ..."; }
};


class PrintCommand : public MenuItem {
public:
    explicit PrintCommand(std::shared_ptr<Client> client) : MenuItem(client) {}
    void handle(CONST_ARG_VECTOR args) override { _client->print(); };
    std::string_view getName() const override { return "print"; }
    std::string_view getDescription() const override { return "Print the current path"; }
};


class ExitCommand : public MenuItem {
    bool& _isExit;
public:
    explicit ExitCommand(std::shared_ptr<Client> client, bool& exitFlag) : MenuItem(client), _isExit(exitFlag) {}
    void handle(CONST_ARG_VECTOR args) override;
    std::string_view getName() const override { return "exit"; }
    std::string_view getDescription() const override { return "Exit the program"; }
};


class HelpCommand : public MenuItem {
    std::unordered_map<std::string, std::unique_ptr<MenuItem>>& _commands;
public:
    HelpCommand(std::shared_ptr<Client> client, std::unordered_map<std::string, std::unique_ptr<MenuItem>>& commands)
        : MenuItem(client), _commands(commands) {}
    void handle(CONST_ARG_VECTOR args) override;
    std::string_view getName() const override { return "help"; }
    std::string_view getDescription() const override { return "Show this help message"; }
};
