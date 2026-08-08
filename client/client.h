#pragma once
#include <iostream>
#include <string>
#include <CLI11.hpp>
#include <boost/asio.hpp>
#include "config.h"

using boost::asio::ip::tcp;
using tcp = boost::asio::ip::tcp;

enum { max_length = 1024 };


class Client {
public:
    Client() : 
        _config(Config()), 
        _io_service(boost::asio::io_service()),
        _resolver(tcp::resolver(_io_service)),
        _socket(tcp::socket(_io_service))
    {}

    void connect();

    Config getConfig() const { return _config; };

private:
    Config _config;
    boost::asio::io_service _io_service;
    tcp::resolver _resolver;
    tcp::socket _socket;
};