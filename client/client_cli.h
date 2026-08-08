#pragma once
#include <iostream>
#include <string>
#include <CLI11.hpp>
#include <boost/asio.hpp>
#include "client.h"
#include "config.h"
#include "menu.h"

class ClientCLI {
    Client _client;
    Config _config;
    bool _isExit{ false };
    std::unordered_map<std::string, std::unique_ptr<MenuItem>> _commands;
public:
    ClientCLI()=default;
    ClientCLI(Client client) : _client(client), _config(client.getConfig()) {};
    void CLIParse(int argc, char** argv);
    void buildCommands();
    void run(int argc, char** argv);
};