/**
 * @file pip_proxy.cpp
 * @brief pip.exe 代理程序
 * 
 * 该程序作为 pip.exe 的代理，负责：
 * 1. 读取配置文件获取当前 Python 版本
 * 2. 构建对应版本的 pip.exe 路径
 * 3. 执行 pip.exe 并转发所有参数
 * 4. 捕获并转发标准输入/输出/错误流
 * 5. 返回子进程的退出码
 */

#include "proxy_utils.h"
#include <iostream>

#ifdef _WIN32
#include <windows.h>
#endif

/**
 * @brief 主函数
 * @param argc 参数个数
 * @param argv 参数数组
 * @return 退出码
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
        std::string logPath = ProxyUtils::getLogFilePath("pip-proxy.log");
        Logger::getInstance().setFileOutput(true, logPath);
        Logger::getInstance().setLogLevel(LogLevel::INFO);
        
        const char* pymVerbose = std::getenv("PYM_VERBOSE");
        bool verboseMode = (pymVerbose != nullptr && std::string(pymVerbose) == "1");
        Logger::getInstance().setConsoleOutput(verboseMode);
        
        LOG_INFO("pip 代理程序启动");

        std::string exeDir = ProxyUtils::getExecutableDirectory();
        LOG_INFO("可执行文件目录: " + exeDir);

        std::string configPath = exeDir + "\\config.json";
        LOG_INFO("配置文件路径: " + configPath);

        auto configManager = ProxyUtils::loadConfig(configPath);

        std::string pythonVersion = ProxyUtils::getCurrentPythonVersion(*configManager);

        std::string pythonPath;
        try {
            pythonPath = configManager->getPythonPath(pythonVersion);
            LOG_INFO("Python 路径: " + pythonPath);
        } catch (const ConfigException& e) {
            ProxyUtils::showErrorAndExit("获取 Python 路径失败: " + std::string(e.what()));
        }

        std::string pipPath = ProxyUtils::getPipPath(pythonPath);

        std::string commandLine = ProxyUtils::buildCommandLine(argc, argv);

        LOG_INFO("命令行参数: " + commandLine);

        exitCode = ProxyUtils::executeProcess(pipPath, commandLine);

        LOG_INFO("pip 代理程序退出，退出码: " + std::to_string(exitCode));

    } catch (const std::exception& e) {
        ProxyUtils::showErrorAndExit("发生未预期的错误: " + std::string(e.what()));
        exitCode = 1;
    } catch (...) {
        ProxyUtils::showErrorAndExit("发生未知错误");
        exitCode = 1;
    }

#ifdef _WIN32
    SetConsoleOutputCP(originalOutputCP);
    SetConsoleCP(originalCP);
#endif
    
    return exitCode;
}