/**
 * @file python_proxy.cpp
 * @brief Python 可执行文件代理程序
 * 
 * 该程序作为 python.exe 的代理，读取配置文件获取当前 Python 版本，
 * 然后启动对应版本的 Python 解释器并传递所有参数。
 */

#include "proxy_utils.h"
#include <iostream>

#ifdef _WIN32
#include <windows.h>
#endif

/**
 * @brief 主函数
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
        std::string logPath = ProxyUtils::getLogFilePath("python-proxy.log");
        Logger::getInstance().setFileOutput(true, logPath);
        Logger::getInstance().setLogLevel(LogLevel::INFO);
        
        const char* pymVerbose = std::getenv("PYM_VERBOSE");
        bool verboseMode = (pymVerbose != nullptr && std::string(pymVerbose) == "1");
        Logger::getInstance().setConsoleOutput(verboseMode);

        LOG_INFO("Python 代理程序启动");

        std::string exeDir = ProxyUtils::getExecutableDirectory();
        LOG_INFO("可执行文件目录: " + exeDir);

        fs::path configPath = fs::path(exeDir) / "config.json";
        LOG_INFO("配置文件路径: " + configPath.string());

        auto configManager = ProxyUtils::loadConfig(configPath.string());

        std::string pythonExePath = ProxyUtils::getPythonExecutablePath(*configManager);

        std::string commandLine = ProxyUtils::buildCommandLine(argc, argv);

        LOG_INFO("传递的命令行参数: " + commandLine);

        exitCode = ProxyUtils::launchProcessWithIO(pythonExePath, commandLine);

        LOG_INFO("Python 代理程序结束，退出码: " + std::to_string(exitCode));

    } catch (const std::exception& e) {
        ProxyUtils::showErrorAndExit("发生未预期的错误: " + std::string(e.what()));
        exitCode = 1;
    } catch (...) {
        ProxyUtils::showErrorAndExit("发生未预期的未知错误");
        exitCode = 1;
    }

#ifdef _WIN32
    SetConsoleOutputCP(originalOutputCP);
    SetConsoleCP(originalCP);
#endif
    
    return exitCode;
}