#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <boost/asio.hpp>
#include "config.h"

using tcp = boost::asio::ip::tcp;

constexpr uint16_t max_length { 1024 };

class Client {
    Config _config;
    boost::asio::io_context _io_context;
    tcp::resolver _resolver;
    tcp::socket _socket;
    
public:
    Client() : 
        _config(Config()), 
        _io_context(boost::asio::io_context()),
        _resolver(tcp::resolver(_io_context)),
        _socket(tcp::socket(_io_context))
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
