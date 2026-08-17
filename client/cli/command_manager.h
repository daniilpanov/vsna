#pragma once
#include <unordered_map>
#include <memory>
#include "menu.h"

class CommandManager {
public:
    CommandManager(std::shared_ptr<Client> client) : _client(client) {};
    void init_commands(std::unordered_map<std::string, std::unique_ptr<MenuItem>>& _commands);

private:
    std::shared_ptr<Client> _client;
};