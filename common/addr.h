#pragma once
#include <string>
#include <sstream>

using STRING_ARG = const std::string&;

class Addr {
public:
    Addr()=default;
    Addr(STRING_ARG, uint16_t);
    Addr(STRING_ARG, STRING_ARG);
    
    STRING_ARG ip() const { return _ip; }
    uint16_t port() const { return _port; }

    void setIp(STRING_ARG);
    void setPort(STRING_ARG);

    const std::string toString() const {
        return _ip + ":" + std::to_string(_port);
    }
    
private:
    std::string _ip;
    uint16_t _port;
};