#pragma once

#include <fstream>
#include <string>
#include <filesystem>
#include <chrono>
#include <ctime>
#include <map>
#include <source_location>
#include <iostream>

namespace Logger
{
    const std::string LOG_FILE{ "data/vsna.log" };

    inline static std::ofstream file;
    inline static bool _enabled;

    enum class Level
    {
        INFO,
        ERROR,
        DEBUG
    };

    void log(Level level, std::string message, const std::source_location location = std::source_location::current());
    void logInfo(std::string message);
    void logError(std::string message);
    void logDebug(std::string message);

    void setEnabled(bool enabled);

    constexpr std::string log_level_to_string(Level level);
}
