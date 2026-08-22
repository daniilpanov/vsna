#include "logger.h"

#include <ctime>

namespace Logger
{
    constexpr std::string log_level_to_string(Level level)
    {
        switch(level)
        {
            case Level::INFO:  return "INFO";
            case Level::ERROR: return "ERROR";
            case Level::DEBUG: return "DEBUG";
        }
        return "unknown";
    }

    void log(Level level, std::string message, const std::source_location location)
    {
        if (!_enabled) return;
        if (!file.is_open())
        {
            file.open(LOG_FILE);
        }
        if (!file.is_open())
        {
            throw std::runtime_error("[!] Cannot open log file");
        }

        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        std::tm* ptm = std::localtime(&time);

        char buffer[10];
        // Format: 20:20:00
        std::strftime(buffer, 10, "%H:%M:%S", ptm);  
        std::string str_time(buffer);

        std::string filename = std::filesystem::path(location.file_name()).filename().string();

        const std::string logText = "[ " + str_time + " : " + log_level_to_string(level) + "] [" + filename + " : " + std::to_string(location.line()) + " ]" + message + "\n";

        std::cout << logText;
        file << logText;

        file.close();
    }

    void setEnabled(bool enabled)
    {
        _enabled = enabled;
    }

    void Logger::logInfo(std::string message)
    {
        log(Level::INFO, message);
    }

    void Logger::logError(std::string message)
    {
        log(Level::ERROR, message);
    }

    void Logger::logDebug(std::string message)
    {
        log(Level::DEBUG, message);
    }
};
