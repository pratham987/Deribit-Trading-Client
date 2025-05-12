#include "logger.hpp"
#include <iostream>

Logger::Logger() {
    std::string logFileName = generateLogFileName();
    m_logFile.open(logFileName, std::ios::out);  
    if (!m_logFile.is_open()) {
        std::cerr << "Failed to open log file: " << logFileName << std::endl;
    }
    log(INFO, "Logger initialized");
}

std::string Logger::generateLogFileName() {
    auto now = std::chrono::system_clock::now();
    auto now_time_t = std::chrono::system_clock::to_time_t(now);
    
    std::stringstream ss;
    ss << "/path/to/logs/" // Replace with your desired log directory
       << std::put_time(std::localtime(&now_time_t), "%Y%m%d_%H%M%S") 
       << ".txt";
    return ss.str();
}

Logger::~Logger() {
    if (m_logFile.is_open()) {
        log(INFO, "Logger shutting down");
        m_logFile.close();
    }
}

std::string Logger::getCurrentTime() {
    auto now = std::chrono::system_clock::now();
    auto now_time_t = std::chrono::system_clock::to_time_t(now);
    auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::stringstream ss;
    ss << std::put_time(std::localtime(&now_time_t), "%Y-%m-%d %H:%M:%S");
    ss << '.' << std::setfill('0') << std::setw(3) << now_ms.count();
    return ss.str();
}

std::string Logger::getLevelString(LogLevel level) {
    switch (level) {
        case INFO: return "INFO";
        case WARNING: return "WARNING";
        case ERROR: return "ERROR";
        case TRADE: return "TRADE";
        case LATENCY: return "LATENCY";
        default: return "UNKNOWN";
    }
}

void Logger::log(LogLevel level, const std::string& message) {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::string formattedMsg = getCurrentTime() + " [" + getLevelString(level) + "] " + message;
    
    if (m_logFile.is_open()) {
        m_logFile << formattedMsg << std::endl;
        m_logFile.flush();
    }
    
    // Also output to console for visibility
    // std::cout << formattedMsg << std::endl;
}

void Logger::logTrade(const std::string& action, const nlohmann::json& tradeData) {
    std::stringstream ss;
    ss << "Trade executed: " << action << " - " << tradeData.dump();
    log(TRADE, ss.str());
}

void Logger::logError(const std::string& context, const std::string& error) {
    std::stringstream ss;
    ss << "[" << context << "] " << error;
    log(ERROR, ss.str());
}

std::chrono::time_point<std::chrono::high_resolution_clock> Logger::startMeasurement(
    const std::string& operation) {
    log(INFO, "Starting measurement: " + operation);
    return std::chrono::high_resolution_clock::now();
}

void Logger::endMeasurement(
    const std::string& operation,
    const std::chrono::time_point<std::chrono::high_resolution_clock>& startTime) {
    
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count();
    
    std::stringstream ss;
    ss << operation << " completed in " << duration << " microseconds (" 
       << static_cast<double>(duration) / 1000.0 << " ms)";
    log(LATENCY, ss.str());
}