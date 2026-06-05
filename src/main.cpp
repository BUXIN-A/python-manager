/**
 * @file main.cpp
 * @brief Python 版本管理器 (pym) 命令行工具主入口
 * 
 * 提供命令行接口来管理多个 Python 版本
 */

#include <iostream>
#include <string>
#include <filesystem>
#include "config_manager.h"
#include "logger.h"
#include "command_handler.h"

#ifdef _WIN32
#include <windows.h>
// 取消 Windows.h 中 ERROR 宏的定义，避免与 CommandResult::ERROR 冲突
#ifdef ERROR
#undef ERROR
#endif

/**
 * @brief 获取可执行文件所在目录
 * @return 可执行文件所在目录的路径
 */
std::string getExecutableDirectory() {
    char buffer[MAX_PATH] = {0};
    DWORD result = GetModuleFileNameA(NULL, buffer, MAX_PATH);
    
    if (result == 0) {
        LOG_ERROR("GetModuleFileNameA failed: " + std::to_string(GetLastError()));
        return "";
    }
    
    if (result == MAX_PATH && GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
        LOG_ERROR("Path too long, exceeds MAX_PATH");
        return "";
    }
    
    std::string::size_type pos = std::string(buffer).find_last_of("\\/");
    if (pos == std::string::npos) {
        return "";
    }
    return std::string(buffer).substr(0, pos);
}

/**
 * @brief 获取日志文件路径
 * @param logFileName 日志文件名
 * @return 日志文件的完整路径（位于可执行文件目录/logs/下）
 */
std::string getLogFilePath(const std::string& logFileName) {
    std::string exeDir = getExecutableDirectory();
    std::filesystem::path logsDir = std::filesystem::path(exeDir) / "logs";
    
    if (!std::filesystem::exists(logsDir)) {
        std::error_code ec;
        if (!std::filesystem::create_directory(logsDir, ec)) {
            LOG_ERROR("无法创建 logs 目录: " + logsDir.string() + ", 错误: " + ec.message());
            return logFileName;
        }
    }
    
    std::filesystem::path logPath = logsDir / logFileName;
    return logPath.string();
}
#endif

/**
 * @brief 主函数入口
 * @param argc 命令行参数数量
 * @param argv 命令行参数数组
 * @return 程序退出码
 */
int main(int argc, char* argv[]) {
#ifdef _WIN32
    UINT originalOutputCP = GetConsoleOutputCP();
    UINT originalCP = GetConsoleCP();
    
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif
    
    int exitCode = 0;
    
    try {
        std::string logPath = getLogFilePath("python-manager.log");
        Logger::getInstance().setFileOutput(true, logPath);
        LOG_INFO("Python 版本管理器启动");

        std::string exeDir = getExecutableDirectory();
        LOG_INFO("可执行文件目录: " + exeDir);

        std::string configPath = exeDir + "\\config.json";
        LOG_INFO("配置文件路径: " + configPath);

        ConfigManager configManager(configPath);
        
        try {
            configManager.loadConfig();
            LOG_INFO("配置文件加载成功");
        } catch (const ConfigException& e) {
            LOG_WARNING("配置文件加载失败，将使用默认配置: " + std::string(e.what()));
        }

        CommandHandler commandHandler(configManager);
        
        CommandResult result = commandHandler.parseAndExecute(argc, argv);
        
        switch (result) {
            case CommandResult::SUCCESS:
                LOG_INFO("命令执行成功");
                exitCode = 0;
                break;
            case CommandResult::ERROR:
                LOG_ERROR("命令执行失败");
                exitCode = 1;
                break;
            case CommandResult::EXIT:
                exitCode = 0;
                break;
        }
        
    } catch (const ConfigException& e) {
        LOG_ERROR("配置错误: " + std::string(e.what()));
        std::cerr << "配置错误: " << e.what() << std::endl;
        exitCode = 1;
    } catch (const std::exception& e) {
        LOG_ERROR("未知错误: " + std::string(e.what()));
        std::cerr << "未知错误: " << e.what() << std::endl;
        exitCode = 1;
    }
    
#ifdef _WIN32
    SetConsoleOutputCP(originalOutputCP);
    SetConsoleCP(originalCP);
#endif
    
    return exitCode;
}