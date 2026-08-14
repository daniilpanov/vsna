#pragma once
#include <iostream>
#include <string>
#include <unordered_map>
#include <memory>
#include <libs/CLI11.hpp>
#include <boost/asio.hpp>
#include "client.h"
#include "config.h"
#include "menu.h"
#include "utils.h"

class ClientCLI {
    std::shared_ptr<Client> _client;
    bool _isExit{ false };
    std::unordered_map<std::string, std::unique_ptr<MenuItem>> _commands;
public:
    ClientCLI() : _client(std::make_shared<Client>()) {}
    void CLIParse(int argc, char** argv);
    void buildCommands();
    void run(int argc, char** argv);
};
