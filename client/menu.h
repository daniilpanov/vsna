#pragma once
#include <unordered_map>
#include <memory>
#include <string>
#include <vector>
#include <iostream>
#include <algorithm>
#include <sstream>
#include <boost/beast/websocket.hpp>
#include <boost/beast/core.hpp>
#include "config.h"
#include "client.h"

using ARG_VECTOR = std::vector<std::string>;
using STRING_ARG = const std::string&;

class MenuItem{
public:
    virtual ~MenuItem()=default;
    virtual void handle(const ARG_VECTOR&)=0;
    virtual const char* getName() const = 0;
    virtual const char* getDescription() const = 0;
    virtual const char* getUsage() const { return ""; }
};

class ConnectCommand : public MenuItem {
    Client* _client;
public:
    ConnectCommand(Client* client) : _client(client) {}
    void handle(const ARG_VECTOR&) override;
    const char* getName() const override { return "connect"; }
    const char* getDescription() const override { return "Connect to the server"; }
    const char* getUsage() const override { return "<ip:port>"; }
};

class ShowPathCommand : public MenuItem {
    const Config& config;
public:
    ShowPathCommand(const Config& config) : config(config) {}
    void handle(const ARG_VECTOR&) override;
    const char* getName() const override { return "path"; }
    const char* getDescription() const override { return "Show the server path"; }
    const char* getUsage() const override { return "[name]"; }
};

class MyPathCommand : public MenuItem {
    const Config& _config;
public:
    MyPathCommand(const Config& config) : _config(config) {}
    void handle(const ARG_VECTOR&) override;
    const char* getName() const override { return "mypath"; }
    const char* getDescription() const override { return "Show the client path"; }
    const char* getUsage() const override { return "[name]"; }
};

class SendFilesCommand : public MenuItem {
    const Config& _config;
public:
    SendFilesCommand(const Config& config) : _config(config) {}
    void handle(const ARG_VECTOR&) override;
    const char* getName() const override { return "send"; }
    const char* getDescription() const override { return "Send files to the server"; }
    const char* getUsage() const override { return "<file1> [file2] ..."; }
};

class DownloadCommand : public MenuItem {
    const Config& _config;
public:
    DownloadCommand(const Config& config) : _config(config) {}
    void handle(const ARG_VECTOR&) override;
    const char* getName() const override { return "download"; }
    const char* getDescription() const override { return "Download files from the server"; }
    const char* getUsage() const override { return "<file1 | path1> [file2] ..."; }
};

class ExitCommand : public MenuItem {
    bool& _isExit;
public:
    ExitCommand(bool& exitFlag) : _isExit(exitFlag) {}
    void handle(const ARG_VECTOR&) override;
    const char* getName() const override { return "exit"; }
    const char* getDescription() const override { return "Exit the program"; }
};

class PrintCommand : public MenuItem {
    const Config& _config;
public:
    PrintCommand(const Config& config) : _config(config) {}
    void handle(const ARG_VECTOR&) override;
    const char* getName() const override { return "print"; }
    const char* getDescription() const override { return "Print the current path"; }
};

class HelpCommand : public MenuItem {
    std::unordered_map<std::string, std::unique_ptr<MenuItem>>& _commands;
public:
    HelpCommand(std::unordered_map<std::string, std::unique_ptr<MenuItem>>& commands)
        : _commands(commands) {}
    void handle(const ARG_VECTOR&) override;
    const char* getName() const override { return "help"; }
    const char* getDescription() const override { return "Show this help message"; }
};

ARG_VECTOR splitArgs(STRING_ARG input);
std::string toLowerCase(STRING_ARG str);