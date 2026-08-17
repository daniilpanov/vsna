#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <boost/asio.hpp>
#include "config.h"
#include "utils.h"
#include "session.h"

class Client {
    Config _config;
    boost::asio::io_context _io_context;
    tcp::resolver _resolver;

    bool _is_exit { false }; // TODO: Can be replaced?
    
public:
    Client() : 
        _config(), 
        _io_context(),
        _resolver(_io_context)
    {}

    void setConfig(const Config& config) { _config = config; }
    Config getConfig() const { return _config; }

    void connect(CONST_ARG_VECTOR);
    void print() const;
    void showPath(CONST_ARG_VECTOR) const;
    void myPath(CONST_ARG_VECTOR) const;
    void sendFiles(CONST_ARG_VECTOR);
    void download(CONST_ARG_VECTOR);
    void exit() { _is_exit = true; };
    bool should_be_closed() { return _is_exit; }
};
