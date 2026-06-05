#include "proxy_utils.h"
#include <iostream>
#include <vector>

// HandleGuard 实现
HandleGuard::HandleGuard(HANDLE handle)
    : handle_(handle) {
}

HandleGuard::~HandleGuard() {
    if (handle_ != nullptr && handle_ != INVALID_HANDLE_VALUE) {
        CloseHandle(handle_);
    }
}

HandleGuard::HandleGuard(HandleGuard&& other) noexcept
    : handle_(other.handle_) {
    other.handle_ = nullptr;
}

HandleGuard& HandleGuard::operator=(HandleGuard&& other) noexcept {
    if (this != &other) {
        if (handle_ != nullptr && handle_ != INVALID_HANDLE_VALUE) {
            CloseHandle(handle_);
        }
        handle_ = other.handle_;
        other.handle_ = nullptr;
    }
    return *this;
}

HANDLE HandleGuard::get() const {
    return handle_;
}

bool HandleGuard::isValid() const {
    return handle_ != nullptr && handle_ != INVALID_HANDLE_VALUE;
}

void HandleGuard::reset(HANDLE handle) {
    if (handle_ != nullptr && handle_ != INVALID_HANDLE_VALUE) {
        CloseHandle(handle_);
    }
    handle_ = handle;
}

HANDLE HandleGuard::release() {
    HANDLE handle = handle_;
    handle_ = nullptr;
    return handle;
}

// ProxyUtils 实现
std::string ProxyUtils::getExecutableDirectory() {
    char buffer[MAX_PATH] = {0};  // 初始化为0，避免垃圾数据
    DWORD result = GetModuleFileNameA(NULL, buffer, MAX_PATH);
    
    // 检查GetModuleFileNameA是否成功
    if (result == 0) {
        showErrorAndExit("GetModuleFileNameA failed: " + std::to_string(GetLastError()));
    }
    
    // 检查是否路径过长（返回值等于MAX_PATH且GetLastError为ERROR_INSUFFICIENT_BUFFER）
    if (result == MAX_PATH && GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
        showErrorAndExit("Path too long, exceeds MAX_PATH");
    }
    
    std::string::size_type pos = std::string(buffer).find_last_of("\\/");
    if (pos == std::string::npos) {
        showErrorAndExit("Invalid executable path");
    }
    return std::string(buffer).substr(0, pos);
}

bool ProxyUtils::fileExists(const std::string& path) {
    return fs::exists(path);
}

bool ProxyUtils::directoryExists(const std::string& path) {
    return fs::exists(path) && fs::is_directory(path);
}

void ProxyUtils::showErrorAndExit(const std::string& message, int exitCode) {
    std::cerr << "错误: " << message << std::endl;
    LOG_ERROR(message);
    exit(exitCode);
}

std::unique_ptr<ConfigManager> ProxyUtils::loadConfig(const std::string& configPath) {
    // 创建配置管理器（构造函数会自动创建默认配置文件如果不存在）
    auto configManager = std::make_unique<ConfigManager>(configPath);
    try {
        configManager->loadConfig();
        LOG_INFO("配置文件加载成功");
    } catch (const ConfigException& e) {
        showErrorAndExit("配置文件加载失败: " + std::string(e.what()));
    }
    
    return configManager;
}

std::string ProxyUtils::getCurrentPythonVersion(ConfigManager& configManager) {
    std::string pythonVersion;
    try {
        pythonVersion = configManager.getCurrentVersion();
        LOG_INFO("当前 Python 版本: " + pythonVersion);
    } catch (const ConfigException& e) {
        showErrorAndExit("获取当前 Python 版本失败: " + std::string(e.what()));
    }
    
    if (pythonVersion.empty()) {
        showErrorAndExit("当前 Python 版本未设置");
    }
    
    return pythonVersion;
}

std::string ProxyUtils::getPythonExecutablePath(ConfigManager& configManager) {
    // 获取当前 Python 版本
    std::string pythonVersion = getCurrentPythonVersion(configManager);
    
    // 获取 Python 路径
    std::string pythonPath;
    try {
        pythonPath = configManager.getPythonPath(pythonVersion);
        LOG_INFO("Python 路径: " + pythonPath);
    } catch (const ConfigException& e) {
        showErrorAndExit("获取 Python 路径失败: " + std::string(e.what()));
    }
    
    // 检查 Python 路径是否存在
    if (!directoryExists(pythonPath)) {
        showErrorAndExit("Python 路径不存在: " + pythonPath);
    }
    
    // 构建 python.exe 完整路径
    fs::path pythonExePath = fs::path(pythonPath) / "python.exe";
    LOG_INFO("Python 可执行文件路径: " + pythonExePath.string());
    
    // 检查 python.exe 是否存在
    if (!fileExists(pythonExePath.string())) {
        showErrorAndExit("Python 可执行文件不存在: " + pythonExePath.string());
    }
    
    return pythonExePath.string();
}

std::string ProxyUtils::getPipPath(const std::string& pythonPath) {
    // 构建 Scripts 目录路径
    std::string scriptsPath = pythonPath + "\\Scripts";
    LOG_INFO("Scripts 目录: " + scriptsPath);
    
    // 检查 Scripts 目录是否存在
    if (!directoryExists(scriptsPath)) {
        showErrorAndExit("Scripts 目录不存在: " + scriptsPath);
    }
    
    // 构建 pip.exe 路径
    std::string pipPath = scriptsPath + "\\pip.exe";
    LOG_INFO("pip.exe 路径: " + pipPath);
    
    // 检查 pip.exe 是否存在
    if (!fileExists(pipPath)) {
        showErrorAndExit("pip.exe 不存在: " + pipPath);
    }
    
    return pipPath;
}

std::string ProxyUtils::buildCommandLine(int argc, char* argv[]) {
    std::string commandLine;
    for (int i = 1; i < argc; ++i) {
        if (i > 1) {
            commandLine += " ";
        }
        // 如果参数包含空格，需要用引号括起来
        std::string arg = argv[i];
        if (arg.find(' ') != std::string::npos) {
            commandLine += "\"" + arg + "\"";
        } else {
            commandLine += arg;
        }
    }
    return commandLine;
}

int ProxyUtils::executeProcess(const std::string& executablePath, const std::string& commandLine) {
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    // 设置启动信息，继承标准输入/输出/错误流
    si.hStdError = GetStdHandle(STD_ERROR_HANDLE);
    si.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    si.dwFlags |= STARTF_USESTDHANDLES;

    // 构建完整的命令行（包括可执行文件路径）
    std::string fullCommandLine = "\"" + executablePath + "\" " + commandLine;
    
    // 检查命令行长度是否超过限制
    if (fullCommandLine.length() > 32767) {  // Windows 命令行最大长度
        showErrorAndExit("命令行过长，超过最大限制");
    }
    
    // 创建可修改的命令行字符串（CreateProcess 需要可修改的字符串）
    std::vector<char> cmdLineBuffer(fullCommandLine.begin(), fullCommandLine.end());
    cmdLineBuffer.push_back('\0');

    LOG_INFO("执行命令: " + fullCommandLine);

    // 创建子进程
    if (!CreateProcessA(
        NULL,                           // 应用程序名（使用命令行中的路径）
        cmdLineBuffer.data(),           // 命令行
        NULL,                           // 进程安全属性
        NULL,                           // 线程安全属性
        TRUE,                           // 继承句柄
        0,                              // 创建标志
        NULL,                           // 环境变量
        NULL,                           // 工作目录
        &si,                            // 启动信息
        &pi                             // 进程信息
    )) {
        DWORD error = GetLastError();
        showErrorAndExit("无法创建进程，错误代码: " + std::to_string(error));
        return -1;
    }

    // 使用 RAII 包装句柄
    HandleGuard processGuard(pi.hProcess);
    HandleGuard threadGuard(pi.hThread);

    // 等待子进程完成（使用合理的超时值，避免无限等待）
    DWORD waitResult = WaitForSingleObject(processGuard.get(), 5000);  // 5秒超时
    
    if (waitResult == WAIT_TIMEOUT) {
        LOG_WARNING("等待进程结束超时（5秒），进程可能仍在运行");
    } else if (waitResult == WAIT_FAILED) {
        LOG_ERROR("等待进程结束失败，错误代码: " + std::to_string(GetLastError()));
    }

    // 获取子进程退出码
    DWORD exitCode = 0;
    if (!GetExitCodeProcess(processGuard.get(), &exitCode)) {
        exitCode = 1;
        LOG_ERROR("无法获取子进程退出码");
    }

    LOG_INFO("子进程退出码: " + std::to_string(exitCode));
    return static_cast<int>(exitCode);
}

int ProxyUtils::launchProcessWithIO(const std::string& executablePath, const std::string& commandLine) {
    SECURITY_ATTRIBUTES saAttr;
    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    saAttr.bInheritHandle = TRUE;
    saAttr.lpSecurityDescriptor = NULL;

    // 创建管道用于标准输出
    HANDLE hStdoutRead, hStdoutWrite;
    if (!CreatePipe(&hStdoutRead, &hStdoutWrite, &saAttr, 0)) {
        showErrorAndExit("无法创建标准输出管道");
    }
    SetHandleInformation(hStdoutRead, HANDLE_FLAG_INHERIT, 0);

    // 创建管道用于标准错误
    HANDLE hStderrRead, hStderrWrite;
    if (!CreatePipe(&hStderrRead, &hStderrWrite, &saAttr, 0)) {
        CloseHandle(hStdoutRead);
        CloseHandle(hStdoutWrite);
        showErrorAndExit("无法创建标准错误管道");
    }
    SetHandleInformation(hStderrRead, HANDLE_FLAG_INHERIT, 0);

    // 创建管道用于标准输入
    HANDLE hStdinRead, hStdinWrite;
    if (!CreatePipe(&hStdinRead, &hStdinWrite, &saAttr, 0)) {
        CloseHandle(hStdoutRead);
        CloseHandle(hStdoutWrite);
        CloseHandle(hStderrRead);
        CloseHandle(hStderrWrite);
        showErrorAndExit("无法创建标准输入管道");
    }
    SetHandleInformation(hStdinWrite, HANDLE_FLAG_INHERIT, 0);

    // 使用 RAII 包装管道句柄
    HandleGuard stdoutReadGuard(hStdoutRead);
    HandleGuard stdoutWriteGuard(hStdoutWrite);
    HandleGuard stderrReadGuard(hStderrRead);
    HandleGuard stderrWriteGuard(hStderrWrite);
    HandleGuard stdinReadGuard(hStdinRead);
    HandleGuard stdinWriteGuard(hStdinWrite);

    // 设置启动信息
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.hStdError = stderrWriteGuard.get();
    si.hStdOutput = stdoutWriteGuard.get();
    si.hStdInput = stdinReadGuard.get();
    si.dwFlags |= STARTF_USESTDHANDLES;
    ZeroMemory(&pi, sizeof(pi));

    // 构建完整的命令行
    std::string fullCommandLine = "\"" + executablePath + "\" " + commandLine;
    
    // 检查命令行长度是否超过限制
    if (fullCommandLine.length() > 32767) {  // Windows 命令行最大长度
        showErrorAndExit("命令行过长，超过最大限制");
    }
    
    // 创建可修改的命令行字符串（CreateProcess 需要可修改的字符串）
    std::vector<char> cmdLineBuffer(fullCommandLine.begin(), fullCommandLine.end());
    cmdLineBuffer.push_back('\0');

    LOG_INFO("启动进程: " + executablePath);
    LOG_INFO("命令行: " + commandLine);

    // 创建子进程
    BOOL success = CreateProcessA(
        NULL,                           // 应用程序名（使用命令行中的路径）
        cmdLineBuffer.data(),           // 命令行
        NULL,                           // 进程安全属性
        NULL,                           // 线程安全属性
        TRUE,                           // 继承句柄
        0,                              // 创建标志
        NULL,                           // 环境变量
        NULL,                           // 当前目录
        &si,                            // 启动信息
        &pi                             // 进程信息
    );

    // 使用 RAII 包装进程和线程句柄
    HandleGuard processGuard(pi.hProcess);
    HandleGuard threadGuard(pi.hThread);

    if (!success) {
        showErrorAndExit("无法创建进程: " + executablePath + ", 错误码: " + std::to_string(GetLastError()));
    }

    LOG_INFO("进程创建成功，PID: " + std::to_string(pi.dwProcessId));

    // 创建线程用于转发 I/O
    // 转发标准输出
    HANDLE hStdoutThread = CreateThread(
        NULL, 0,
        [](LPVOID param) -> DWORD {
            HANDLE hRead = (HANDLE)param;
            char buffer[4096];
            DWORD bytesRead;
            while (ReadFile(hRead, buffer, sizeof(buffer), &bytesRead, NULL) && bytesRead > 0) {
                fwrite(buffer, 1, bytesRead, stdout);
                fflush(stdout);
            }
            return 0;
        },
        stdoutReadGuard.get(), 0, NULL
    );
    
    // 检查线程创建是否成功
    if (hStdoutThread == NULL) {
        LOG_ERROR("无法创建标准输出转发线程");
    }

    // 转发标准错误
    HANDLE hStderrThread = CreateThread(
        NULL, 0,
        [](LPVOID param) -> DWORD {
            HANDLE hRead = (HANDLE)param;
            char buffer[4096];
            DWORD bytesRead;
            while (ReadFile(hRead, buffer, sizeof(buffer), &bytesRead, NULL) && bytesRead > 0) {
                fwrite(buffer, 1, bytesRead, stderr);
                fflush(stderr);
            }
            return 0;
        },
        stderrReadGuard.get(), 0, NULL
    );
    
    // 检查线程创建是否成功
    if (hStderrThread == NULL) {
        LOG_ERROR("无法创建标准错误转发线程");
    }

    // 转发标准输入
    HANDLE hStdinThread = CreateThread(
        NULL, 0,
        [](LPVOID param) -> DWORD {
            HANDLE hWrite = (HANDLE)param;
            char buffer[4096];
            size_t bytesRead;
            while ((bytesRead = fread(buffer, 1, sizeof(buffer), stdin)) > 0) {
                DWORD bytesWritten;
                WriteFile(hWrite, buffer, static_cast<DWORD>(bytesRead), &bytesWritten, NULL);
            }
            return 0;
        },
        stdinWriteGuard.get(), 0, NULL
    );
    
    // 检查线程创建是否成功
    if (hStdinThread == NULL) {
        LOG_ERROR("无法创建标准输入转发线程");
    }

    // 使用 RAII 包装线程句柄
    HandleGuard stdoutThreadGuard(hStdoutThread);
    HandleGuard stderrThreadGuard(hStderrThread);
    HandleGuard stdinThreadGuard(hStdinThread);

    // 等待子进程结束（使用合理的超时值，避免无限等待）
    DWORD waitResult = WaitForSingleObject(processGuard.get(), 5000);  // 5秒超时
    
    if (waitResult == WAIT_TIMEOUT) {
        LOG_WARNING("等待进程结束超时（5秒），进程可能仍在运行");
    } else if (waitResult == WAIT_FAILED) {
        LOG_ERROR("等待进程结束失败，错误代码: " + std::to_string(GetLastError()));
    }

    // 获取退出码
    DWORD exitCode = 0;
    GetExitCodeProcess(processGuard.get(), &exitCode);

    LOG_INFO("进程结束，退出码: " + std::to_string(exitCode));

    // 关闭管道写入端，强制结束 I/O 线程
    // 这样可以避免等待线程超时，快速退出
    stdoutWriteGuard.reset();  // 关闭标准输出写入端
    stderrWriteGuard.reset();  // 关闭标准错误写入端
    stdinReadGuard.reset();    // 关闭标准输入读取端
    
    // 等待 I/O 线程结束（减少等待时间，从1秒改为100ms）
    if (stdoutThreadGuard.isValid()) {
        WaitForSingleObject(stdoutThreadGuard.get(), 100);
    }
    if (stderrThreadGuard.isValid()) {
        WaitForSingleObject(stderrThreadGuard.get(), 100);
    }
    if (stdinThreadGuard.isValid()) {
        WaitForSingleObject(stdinThreadGuard.get(), 100);
    }

    return static_cast<int>(exitCode);
}

std::string ProxyUtils::getLogFilePath(const std::string& logFileName) {
    // 获取可执行文件所在目录
    std::string exeDir = getExecutableDirectory();
    
    // 构建 logs 目录路径
    fs::path logsDir = fs::path(exeDir) / "logs";
    
    // 如果 logs 目录不存在，创建它
    if (!fs::exists(logsDir)) {
        std::error_code ec;
        if (!fs::create_directory(logsDir, ec)) {
            // 如果创建失败，记录错误并返回当前目录下的日志文件路径
            LOG_ERROR("无法创建 logs 目录: " + logsDir.string() + ", 错误: " + ec.message());
            return logFileName;
        }
    }
    
    // 返回日志文件的完整路径
    fs::path logPath = logsDir / logFileName;
    return logPath.string();
}