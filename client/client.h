#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <boost/asio.hpp>
#include "config.h"
#include "utils.h"
#include "session.h"

using tcp = boost::asio::ip::tcp;

class Client {
    Config _config;
    boost::asio::io_service _io_service;
    tcp::resolver _resolver;
    
public:
    Client() : 
        _config(Config()), 
        _io_service(boost::asio::io_service()),
        _resolver(tcp::resolver(_io_service))
    {}

    void setConfig(const Config& config) { _config = config; }
    Config getConfig() const { return _config; }

    void connect(CONST_ARG_VECTOR);
    void print() const;
    void showPath(CONST_ARG_VECTOR) const;
    void myPath(CONST_ARG_VECTOR) const;
    void sendFiles(CONST_ARG_VECTOR);
    void download(CONST_ARG_VECTOR);
};
