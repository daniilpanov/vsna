#pragma once
#include <string>
#include <fstream>
#include <filesystem>
#include <sstream>
#include <stdexcept>
#include <cstdint>
#include <json.hpp>
#include "addr.h"

using STRING_ARG = const std::string&;

using json = nlohmann::json;

class Config {
public:
    Addr addr;
    std::string path;

    Config()=default;
    Config(const Addr&, STRING_ARG);

    static Config loadFromFile(STRING_ARG);
    void setAddr(const Addr&);
    void setPath(STRING_ARG);

    std::string toString() const;
    std::string getAddr() const;
};