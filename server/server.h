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
    boost::asio::io_context _io_context;
    
public:
    Server(): 
        _config(Config()),
        _io_context(boost::asio::io_context())
    {}

    void setConfig(const Config& config) { _config = config; }
    Config getConfig() const { return _config; }
    
    void start();
};