#ifndef LOGGER_H
#define LOGGER_H

#include <string>
#include <mutex>
#include <iostream>
#include <fstream>
#include <chrono>
#include <iomanip>
#include <sstream>

// 取消 Windows.h 中 ERROR 宏的定义，避免与 LogLevel::ERROR 冲突
#ifdef ERROR
#undef ERROR
#endif

/**
 * @brief 日志级别枚举
 */
enum class LogLevel {
    INFO,
    WARNING,
    ERROR
};

/**
 * @brief 日志记录器类
 * 
 * 提供线程安全的日志记录功能，支持不同的日志级别
 */
class Logger {
public:
    /**
     * @brief 获取 Logger 单例实例
     * @return Logger 实例引用
     */
    static Logger& getInstance();

    /**
     * @brief 记录 INFO 级别日志
     * @param message 日志消息
     */
    void info(const std::string& message);

    /**
     * @brief 记录 WARNING 级别日志
     * @param message 日志消息
     */
    void warning(const std::string& message);

    /**
     * @brief 记录 ERROR 级别日志
     * @param message 日志消息
     */
    void error(const std::string& message);

    /**
     * @brief 设置日志级别
     * @param level 日志级别
     */
    void setLogLevel(LogLevel level);

    /**
     * @brief 设置是否输出到文件
     * @param enable 是否启用文件输出
     * @param filename 日志文件名（可选）
     */
    void setFileOutput(bool enable, const std::string& filename = "app.log");

    /**
     * @brief 设置是否输出到控制台
     * @param enable 是否启用控制台输出
     */
    void setConsoleOutput(bool enable);

    // 删除拷贝构造函数和赋值运算符
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

private:
    Logger();
    ~Logger();

    /**
     * @brief 格式化当前时间
     * @return 格式化的时间字符串
     */
    std::string getCurrentTime();

    /**
     * @brief 日志级别转换为字符串
     * @param level 日志级别
     * @return 日志级别字符串
     */
    std::string levelToString(LogLevel level);

    /**
     * @brief 内部日志记录方法
     * @param level 日志级别
     * @param message 日志消息
     */
    void log(LogLevel level, const std::string& message);

    LogLevel currentLevel_;         // 当前日志级别
    std::mutex mutex_;               // 互斥锁，保证线程安全
    bool fileOutput_;                // 是否输出到文件
    bool consoleOutput_;             // 是否输出到控制台
    std::string logFilename_;        // 日志文件名
    std::ofstream logFile_;          // 日志文件流
};

// 便捷宏定义
#define LOG_INFO(msg) Logger::getInstance().info(msg)
#define LOG_WARNING(msg) Logger::getInstance().warning(msg)
#define LOG_ERROR(msg) Logger::getInstance().error(msg)

#endif // LOGGER_H