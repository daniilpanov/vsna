#pragma once
#include <string>
#include <sstream>
#include <boost/asio/ip/address.hpp>
#include "types.h"

class Addr {
    std::string _ip{ "0.0.0.0" };
    std::string _port{ "5555" };
public:
    Addr()=default;
    Addr(STRING_ARG, STRING_ARG);
    
    STRING_ARG ip() const { return _ip; }
    STRING_ARG port() const { return _port; }
    uint16_t portNum() const { return std::stoi(_port); }

    void setIp(STRING_ARG);
    void setPort(STRING_ARG);

    const std::string toString() const {
        return _ip + ":" + _port;
    }
};