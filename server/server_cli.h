#pragma once
#include <iostream>
#include <string>
#include <memory>
#include <libs/CLI11.hpp>
#include <boost/asio.hpp>
#include "server.h"
#include "config.h"
#include "menu.h"
#include "utils.h"

class ServerCLI {
    Server _server;
    
public:
    ServerCLI() = default;
    void CLIParse(int argc, char** argv);
    void run(int argc, char** argv);
};