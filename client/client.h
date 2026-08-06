#pragma once
#include <iostream>
#include <string>
#include <CLI11.hpp>
#include "config.h"
#include "menu.h"

class Client {
private:
    Config _config;
    Menu _menu;
        
public:
    Client() : _config(Config()), _menu(Menu(_config)) {}
    void CLIParse(int argc, char** argv);
    void startCLI() const;
};