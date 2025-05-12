#pragma once

#include <string>
#include <fstream>
#include <chrono>
#include <mutex>
#include <sstream>
#include <iomanip>
#include <nlohmann/json.hpp>

class Logger {
public:
    enum LogLevel {
        INFO,
        WARNING,
        ERROR,
        TRADE,
        LATENCY
    };

    static Logger& getInstance() {
        static Logger instance;
        return instance;
    }

    void log(LogLevel level, const std::string& message);
    void logTrade(const std::string& action, const nlohmann::json& tradeData);
    void logError(const std::string& context, const std::string& error);
    
    // Latency measurement methods
    std::chrono::time_point<std::chrono::high_resolution_clock> startMeasurement(const std::string& operation);
    void endMeasurement(const std::string& operation, 
                        const std::chrono::time_point<std::chrono::high_resolution_clock>& startTime);

private:
    Logger();
    ~Logger();
    std::string getCurrentTime();
    std::string generateLogFileName();
    std::string getLevelString(LogLevel level);

    std::ofstream m_logFile;
    std::mutex m_mutex;
};

// Helper macros for easier use
#define LOG_INFO(msg) Logger::getInstance().log(Logger::INFO, msg)
#define LOG_WARNING(msg) Logger::getInstance().log(Logger::WARNING, msg)
#define LOG_ERROR(msg) Logger::getInstance().log(Logger::ERROR, msg)
#define LOG_TRADE(action, data) Logger::getInstance().logTrade(action, data)
#define LOG_ERROR_CTX(ctx, err) Logger::getInstance().logError(ctx, err)
#define START_MEASUREMENT(op) auto start_##op = Logger::getInstance().startMeasurement(#op)
#define END_MEASUREMENT(op) Logger::getInstance().endMeasurement(#op, start_##op)