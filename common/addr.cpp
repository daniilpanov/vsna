#include "addr.h"

Addr::Addr(STRING_ARG ip, uint16_t port){
    if (port < 0 || port > 65535) {
        throw std::out_of_range("[!] Port out of range (0-65535): " + std::to_string(port));
    }
    setPort(std::to_string(port));
    setIp(ip);
}

Addr::Addr(STRING_ARG ip, STRING_ARG port){
    setIp(ip);
    setPort(port);
}

void Addr::setIp(STRING_ARG ip) {
    std::stringstream ss(ip);
    std::string part;
    uint8_t index{ 0 };
    
    while (std::getline(ss, part, '.') && index < 4) {
        if (part.empty()) {
            throw std::invalid_argument("[!] Empty IP part at position " + std::to_string(index));
        }
        
        for (char c : part) {
            if (!std::isdigit(static_cast<uint8_t>(c))) {
                throw std::invalid_argument("[!] Non-digit character in IP part: " + part);
            }
        }
        
        int num = std::stoi(part);
        if (num < 0 || num > 255) {
            throw std::out_of_range("[!] IP part out of range (0-255): " + part);
        }
        index++;
    }
    
    if (index != 4) {
        throw std::invalid_argument("[!] Invalid IPv4 format: expected 4 parts, got " + 
                                   std::to_string(index) + " in: " + ip);
    }
    this->_ip = ip;
}

void Addr::setPort(STRING_ARG port) {
    if (port.empty()) {
        throw std::invalid_argument("[!] Port cannot be empty");
    }
    int port_num = std::stoi(port);
    if (port_num < 0 || port_num > 65535) {
        throw std::out_of_range("[!] Port out of range (0-65535): " + port);
    }
    this->_port = port_num;
}

void Addr::setPort(const uint16_t port) {
    if (port < 0 || port > 65535) {
        throw std::out_of_range("[!] Port out of range (0-65535): " + std::to_string(port));
    }
    this->_port = port;
}