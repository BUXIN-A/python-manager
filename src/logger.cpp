#include "logger.h"
#include <iostream>

#ifdef _WIN32
#include <windows.h>
#include <filesystem>
// 取消 Windows.h 中 ERROR 宏的定义，避免与 LogLevel::ERROR 冲突
#ifdef ERROR
#undef ERROR
#endif
#endif

Logger::Logger() 
    : currentLevel_(LogLevel::INFO)
    , fileOutput_(false)
    , consoleOutput_(true)
    , logFilename_("app.log") {
}

Logger::~Logger() {
    if (logFile_.is_open()) {
        logFile_.flush();
    }
}

Logger& Logger::getInstance() {
    static Logger instance;
    return instance;
}

void Logger::info(const std::string& message) {
    log(LogLevel::INFO, message);
}

void Logger::warning(const std::string& message) {
    log(LogLevel::WARNING, message);
}

void Logger::error(const std::string& message) {
    log(LogLevel::ERROR, message);
}

void Logger::setLogLevel(LogLevel level) {
    std::lock_guard<std::mutex> lock(mutex_);
    currentLevel_ = level;
}

void Logger::setFileOutput(bool enable, const std::string& filename) {
    std::lock_guard<std::mutex> lock(mutex_);
    fileOutput_ = enable;
    logFilename_ = filename;
    
    if (fileOutput_) {
        if (logFile_.is_open()) {
            logFile_.close();
        }
        
        // 创建日志目录（如果不存在）
        #ifdef _WIN32
        std::filesystem::path logPath(logFilename_);
        std::filesystem::path logDir = logPath.parent_path();
        if (!logDir.empty() && !std::filesystem::exists(logDir)) {
            try {
                std::filesystem::create_directories(logDir);
            } catch (const std::filesystem::filesystem_error& e) {
                // 目录创建失败，无法记录日志
                fileOutput_ = false;
                return;
            }
        }
        #endif
        
        logFile_.open(logFilename_, std::ios::app);
        if (!logFile_.is_open()) {
            fileOutput_ = false;
        }
    } else {
        if (logFile_.is_open()) {
            logFile_.close();
        }
    }
}

void Logger::setConsoleOutput(bool enable) {
    std::lock_guard<std::mutex> lock(mutex_);
    consoleOutput_ = enable;
}

std::string Logger::getCurrentTime() {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::stringstream ss;
#ifdef _WIN32
    std::tm tm_time;
    localtime_s(&tm_time, &time);
    ss << std::put_time(&tm_time, "%Y-%m-%d %H:%M:%S");
#else
    std::tm tm_time;
    localtime_r(&time, &tm_time);
    ss << std::put_time(&tm_time, "%Y-%m-%d %H:%M:%S");
#endif
    ss << '.' << std::setfill('0') << std::setw(3) << ms.count();
    
    return ss.str();
}

std::string Logger::levelToString(LogLevel level) {
    switch (level) {
        case LogLevel::INFO:
            return "INFO";
        case LogLevel::WARNING:
            return "WARNING";
        case LogLevel::ERROR:
            return "ERROR";
        default:
            return "UNKNOWN";
    }
}

#ifdef _WIN32
static void printUtf8ToConsole(const std::string& message) {
    int size = MultiByteToWideChar(CP_UTF8, 0, message.c_str(), -1, NULL, 0);
    if (size <= 0) {
        std::cout << message;
        return;
    }
    
    std::wstring wMessage(size - 1, 0);
    int result = MultiByteToWideChar(CP_UTF8, 0, message.c_str(), -1, &wMessage[0], size);
    if (result <= 0) {
        std::cout << message;
        return;
    }
    
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hConsole == INVALID_HANDLE_VALUE || hConsole == NULL) {
        std::cout << message;
        return;
    }
    
    DWORD mode;
    if (!GetConsoleMode(hConsole, &mode)) {
        std::cout << message;
        return;
    }
    
    DWORD charsWritten;
    BOOL writeResult = WriteConsoleW(hConsole, wMessage.c_str(), static_cast<DWORD>(wMessage.length()), &charsWritten, NULL);
    if (!writeResult) {
        std::cout << message;
    }
}
#endif

void Logger::log(LogLevel level, const std::string& message) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::stringstream ss;
    ss << "[" << getCurrentTime() << "] "
       << "[" << levelToString(level) << "] "
       << message << std::endl;
    
    std::string logMessage = ss.str();
    
    if (consoleOutput_) {
#ifdef _WIN32
        printUtf8ToConsole(logMessage);
#else
        std::cout << logMessage;
#endif
    }
    
    if (fileOutput_ && logFile_.is_open()) {
        logFile_ << logMessage;
        if (level == LogLevel::ERROR) {
            logFile_.flush();
        }
    }
}