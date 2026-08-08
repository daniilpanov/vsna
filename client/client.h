#pragma once
#include <iostream>
#include <string>
#include <CLI11.hpp>
#include "config.h"
#include "menu.h"


class Client {
public:
    Client() : _config(Config()) {}
    Config getConfig() const { return _config; };

private:
    Config _config;

};


class ClientCLI {
public:
    ClientCLI()=default;
    ClientCLI(const Client* client) : _client(client), _config(client->getConfig()) { this->buildCommands(); }
    void CLIParse(int argc, char** argv);
    void buildCommands();
    void run(int argc, char** argv);

private:
    const Client* _client;
    Config _config;
    bool _isExit{ false };
    std::unordered_map<std::string, std::unique_ptr<MenuItem>> _commands;
};