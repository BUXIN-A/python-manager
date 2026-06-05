/**
 * @file pythonw_proxy.cpp
 * @brief Pythonw 可执行文件代理程序
 * 
 * 该程序作为 pythonw.exe 的代理，读取配置文件获取当前 Python 版本，
 * 然后启动对应版本的 Pythonw 解释器并传递所有参数。
 * 
 * 与 python_proxy 不同，此程序不创建控制台窗口，适用于 GUI 应用。
 */

#include "proxy_utils.h"
#include <shellapi.h>

/**
 * @brief 显示错误消息框（用于无控制台程序）
 * @param message 错误消息
 * @param exitCode 退出码
 */
void showErrorMessageBox(const std::string& message, int exitCode = 1) {
    int size = MultiByteToWideChar(CP_UTF8, 0, message.c_str(), -1, NULL, 0);
    if (size <= 0) {
        MessageBoxW(NULL, L"Unknown error", L"Pythonw Proxy Error", MB_OK | MB_ICONERROR);
        LOG_ERROR("MultiByteToWideChar failed for error message");
        exit(exitCode);
    }
    
    std::wstring wMessage(size - 1, 0);
    int result = MultiByteToWideChar(CP_UTF8, 0, message.c_str(), -1, &wMessage[0], size);
    if (result <= 0) {
        MessageBoxW(NULL, L"Unknown error", L"Pythonw Proxy Error", MB_OK | MB_ICONERROR);
        LOG_ERROR("MultiByteToWideChar failed for error message");
        exit(exitCode);
    }
    
    MessageBoxW(NULL, wMessage.c_str(), L"Pythonw Proxy Error", MB_OK | MB_ICONERROR);
    LOG_ERROR(message);
    exit(exitCode);
}

/**
 * @brief 启动子进程（无控制台窗口）
 * @param executablePath 可执行文件路径
 * @param commandLine 命令行参数
 * @return 子进程退出码
 */
int launchProcessNoConsole(const std::string& executablePath, const std::string& commandLine) {
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    ZeroMemory(&pi, sizeof(pi));

    std::string fullCommandLine = "\"" + executablePath + "\" " + commandLine;
    
    if (fullCommandLine.length() > 32767) {
        showErrorMessageBox("命令行过长，超过最大限制");
    }
    
    std::vector<char> cmdLineBuffer(fullCommandLine.begin(), fullCommandLine.end());
    cmdLineBuffer.push_back('\0');

    LOG_INFO("启动进程: " + executablePath);
    LOG_INFO("命令行: " + commandLine);

    BOOL success = CreateProcessA(
        NULL,
        cmdLineBuffer.data(),
        NULL,
        NULL,
        FALSE,
        CREATE_NO_WINDOW,
        NULL,
        NULL,
        &si,
        &pi
    );

    HandleGuard processGuard(pi.hProcess);
    HandleGuard threadGuard(pi.hThread);

    if (!success) {
        showErrorMessageBox("无法创建进程: " + executablePath + ", 错误码: " + std::to_string(GetLastError()));
    }

    LOG_INFO("进程创建成功，PID: " + std::to_string(pi.dwProcessId));

    DWORD waitResult = WaitForSingleObject(processGuard.get(), 5000);
    
    if (waitResult == WAIT_TIMEOUT) {
        LOG_WARNING("等待进程结束超时（5秒），进程可能仍在运行");
    }

    DWORD exitCode = 0;
    GetExitCodeProcess(processGuard.get(), &exitCode);

    LOG_INFO("进程结束，退出码: " + std::to_string(exitCode));

    return static_cast<int>(exitCode);
}

/**
 * @brief 主函数
 */
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    int argc = 0;
    LPWSTR* argvW = CommandLineToArgvW(GetCommandLineW(), &argc);
    
    if (argvW == NULL) {
        showErrorMessageBox("无法解析命令行参数");
        return 1;
    }
    
    std::vector<std::string> args;
    std::vector<char*> argv;
    for (int i = 0; i < argc; ++i) {
        int size = WideCharToMultiByte(CP_UTF8, 0, argvW[i], -1, NULL, 0, NULL, NULL);
        if (size <= 0) {
            continue;
        }
        std::string arg(size - 1, 0);
        WideCharToMultiByte(CP_UTF8, 0, argvW[i], -1, &arg[0], size, NULL, NULL);
        args.push_back(arg);
        argv.push_back(const_cast<char*>(args.back().c_str()));
    }
    
    LocalFree(argvW);

    try {
        std::string logPath = ProxyUtils::getLogFilePath("pythonw-proxy.log");
        Logger::getInstance().setFileOutput(true, logPath);
        Logger::getInstance().setLogLevel(LogLevel::INFO);
        
        const char* pymVerbose = std::getenv("PYM_VERBOSE");
        bool verboseMode = (pymVerbose != nullptr && std::string(pymVerbose) == "1");
        Logger::getInstance().setConsoleOutput(verboseMode);

        LOG_INFO("Pythonw 代理程序启动");

        std::string exeDir = ProxyUtils::getExecutableDirectory();
        LOG_INFO("可执行文件目录: " + exeDir);

        fs::path configPath = fs::path(exeDir) / "config.json";
        LOG_INFO("配置文件路径: " + configPath.string());

        auto configManager = ProxyUtils::loadConfig(configPath.string());

        std::string pythonVersion = ProxyUtils::getCurrentPythonVersion(*configManager);
        
        std::string pythonPath;
        try {
            pythonPath = configManager->getPythonPath(pythonVersion);
            LOG_INFO("Python 路径: " + pythonPath);
        } catch (const ConfigException& e) {
            showErrorMessageBox("获取 Python 路径失败: " + std::string(e.what()));
        }

        if (!ProxyUtils::directoryExists(pythonPath)) {
            showErrorMessageBox("Python 路径不存在: " + pythonPath);
        }

        fs::path pythonwExePath = fs::path(pythonPath) / "pythonw.exe";
        LOG_INFO("Pythonw 可执行文件路径: " + pythonwExePath.string());

        if (!ProxyUtils::fileExists(pythonwExePath.string())) {
            showErrorMessageBox("Pythonw 可执行文件不存在: " + pythonwExePath.string());
        }

        std::string commandLine;
        for (int i = 1; i < argc; ++i) {
            if (i > 1) {
                commandLine += " ";
            }
            std::string arg = args[i];
            if (arg.find(' ') != std::string::npos) {
                commandLine += "\"" + arg + "\"";
            } else {
                commandLine += arg;
            }
        }

        LOG_INFO("传递的命令行参数: " + commandLine);

        int exitCode = launchProcessNoConsole(pythonwExePath.string(), commandLine);

        LOG_INFO("Pythonw 代理程序结束，退出码: " + std::to_string(exitCode));

        return exitCode;

    } catch (const std::exception& e) {
        showErrorMessageBox("发生未预期的错误: " + std::string(e.what()));
    } catch (...) {
        showErrorMessageBox("发生未预期的未知错误");
    }

    return 1;
}