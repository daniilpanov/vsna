#pragma once
#include <iostream>
#include <thread>
#include <boost/beast/websocket.hpp>
#include <boost/beast/core.hpp>
#include "config.h"
#include "utils.h"
#include "session.h"

using tcp = boost::asio::ip::tcp;
using socket_ptr = boost::shared_ptr<tcp::socket>;

class Server {
    Config _config;
    boost::asio::io_service _io_service;
    
public:
    Server(): 
        _config(Config()),
        _io_service(boost::asio::io_service())
    {}

    void setConfig(const Config& config) { _config = config; }
    Config getConfig() const { return _config; }
    
    void start();
};