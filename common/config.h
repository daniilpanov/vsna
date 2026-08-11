#pragma once
#include <fstream>
#include <filesystem>
#include <stdexcept>
#include <cstdint>
#include <string>
#include <libs/json.hpp>
#include "addr.h"
#include "types.h"

using json = nlohmann::json;

class Config {
    Addr _addr;
    std::string _path;
public:
    Config()=default;
    Config(const Addr&, STRING_ARG);

    static Config loadFromFile(STRING_ARG);
    void setAddr(const Addr&);
    void setPath(STRING_ARG);

    Addr getAddr() const { return this->_addr; }
    std::string getPath() const { return this->_path; }

    std::string toString() const;
};