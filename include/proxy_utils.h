#ifndef PROXY_UTILS_H
#define PROXY_UTILS_H

#include <string>
#include <vector>
#include <memory>
#include <filesystem>
#include "config_manager.h"
#include "logger.h"

// 禁用 Windows.h 中的 min/max 宏，避免与标准库冲突
#ifndef NOMINMAX
#define NOMINMAX
#endif
// 定义 Windows 版本，支持 Windows 7 及以上
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0601
#endif

#include <windows.h>

namespace fs = std::filesystem;

/**
 * @brief Windows 句柄 RAII 包装类
 * 
 * 自动管理 Windows 句柄的生命周期，确保句柄正确关闭
 */
class HandleGuard {
public:
    /**
     * @brief 构造函数
     * @param handle Windows 句柄
     */
    explicit HandleGuard(HANDLE handle = nullptr);
    
    /**
     * @brief 析构函数，自动关闭句柄
     */
    ~HandleGuard();
    
    // 禁止拷贝
    HandleGuard(const HandleGuard&) = delete;
    HandleGuard& operator=(const HandleGuard&) = delete;
    
    // 允许移动
    HandleGuard(HandleGuard&& other) noexcept;
    HandleGuard& operator=(HandleGuard&& other) noexcept;
    
    /**
     * @brief 获取句柄
     * @return Windows 句柄
     */
    HANDLE get() const;
    
    /**
     * @brief 检查句柄是否有效
     * @return 是否有效
     */
    bool isValid() const;
    
    /**
     * @brief 重置句柄
     * @param handle 新句柄
     */
    void reset(HANDLE handle = nullptr);
    
    /**
     * @brief 释放句柄（不关闭）
     * @return 释放的句柄
     */
    HANDLE release();

private:
    HANDLE handle_;
};

/**
 * @brief 代理程序工具类
 * 
 * 提供代理程序的公共功能，包括：
 * - 获取可执行文件目录
 * - 加载配置文件
 * - 执行子进程
 * - 转发 I/O 流
 */
class ProxyUtils {
public:
    /**
     * @brief 获取可执行文件所在目录
     * @return 可执行文件所在目录的路径
     */
    static std::string getExecutableDirectory();
    
    /**
     * @brief 检查文件是否存在
     * @param path 文件路径
     * @return 文件是否存在
     */
    static bool fileExists(const std::string& path);
    
    /**
     * @brief 检查目录是否存在
     * @param path 目录路径
     * @return 目录是否存在
     */
    static bool directoryExists(const std::string& path);
    
    /**
     * @brief 显示错误消息并退出
     * @param message 错误消息
     * @param exitCode 退出码
     */
    static void showErrorAndExit(const std::string& message, int exitCode = 1);
    
    /**
     * @brief 加载配置文件
     * @param configPath 配置文件路径
     * @return 配置管理器
     */
    static std::unique_ptr<ConfigManager> loadConfig(const std::string& configPath);
    
    /**
     * @brief 获取当前 Python 版本
     * @param configManager 配置管理器
     * @return 当前 Python 版本
     */
    static std::string getCurrentPythonVersion(ConfigManager& configManager);
    
    /**
     * @brief 获取 Python 可执行文件路径
     * @param configManager 配置管理器
     * @return Python 可执行文件路径
     */
    static std::string getPythonExecutablePath(ConfigManager& configManager);
    
    /**
     * @brief 构建 pip.exe 路径
     * @param pythonPath Python 安装路径
     * @return pip.exe 路径
     */
    static std::string getPipPath(const std::string& pythonPath);
    
    /**
     * @brief 构建命令行参数字符串
     * @param argc 参数数量
     * @param argv 参数数组
     * @return 命令行参数字符串
     */
    static std::string buildCommandLine(int argc, char* argv[]);
    
    /**
     * @brief 执行子进程并等待其完成（简单版本）
     * @param executablePath 可执行文件路径
     * @param commandLine 命令行参数
     * @return 子进程退出码
     */
    static int executeProcess(const std::string& executablePath, const std::string& commandLine);
    
    /**
     * @brief 执行子进程并转发 I/O 流（完整版本）
     * @param executablePath 可执行文件路径
     * @param commandLine 命令行参数
     * @return 子进程退出码
     */
    static int launchProcessWithIO(const std::string& executablePath, const std::string& commandLine);
    
    /**
     * @brief 获取日志文件路径
     * @param logFileName 日志文件名（如 "python-manager.log"）
     * @return 日志文件的完整路径（位于可执行文件目录/logs/下）
     * 
     * 该方法会自动创建 logs 目录（如果不存在）
     */
    static std::string getLogFilePath(const std::string& logFileName);
};

#endif // PROXY_UTILS_H