#pragma once
#include <iostream>
#include <thread>
#include <boost/beast/websocket.hpp>
#include <boost/beast/core.hpp>
#include "config.h"

using tcp = boost::asio::ip::tcp;

class Server {
    Config _config;
    
public:
    Server(): _config(Config()) {}

    void setConfig(const Config& config) { _config = config; }
    Config getConfig() const { return _config; }
    
    void start();
};