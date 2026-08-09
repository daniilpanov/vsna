#pragma once
#include <iostream>
#include <string>
#include <unordered_map>
#include <memory>
#include <CLI11.hpp>
#include <boost/asio.hpp>
#include "client.h"
#include "config.h"
#include "menu.h"
#include "utils.h"

class ClientCLI {
    Client _client;
    Config _config;
    bool _isExit{ false };
    std::unordered_map<std::string, std::unique_ptr<MenuItem>> _commands;
public:
    ClientCLI() = default;
    void CLIParse(int argc, char** argv);
    void buildCommands();
    void run(int argc, char** argv);
};
