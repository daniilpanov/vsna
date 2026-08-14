#pragma once
#include <iostream>
#include <string>
#include <memory>
#include <libs/CLI11.hpp>
#include <boost/asio.hpp>
#include "server.h"
#include "config.h"
#include "utils.h"

class ServerCLI {
    std::shared_ptr<Server> _server;
    
public:
    ServerCLI() : _server(std::make_shared<Server>()) {}
    void CLIParse(int argc, char** argv);
    void run(int argc, char** argv);
};