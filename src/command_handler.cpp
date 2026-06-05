#include "command_handler.h"
#include <iostream>
#include <iomanip>
#include <cstdio>
#include <array>
#include <filesystem>
#include <algorithm>
#include <cctype>
#include <sstream>
#include <memory>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <shellapi.h>
#include <shobjidl.h>
#ifdef ERROR
#undef ERROR
#endif
#endif

namespace fs = std::filesystem;

#ifdef _WIN32
/**
 * @brief 辅助函数：将 UTF-8 字符串输出到控制台
 * @param message UTF-8 编码的消息
 */
static void printUtf8(const std::string& message) {
    int size = MultiByteToWideChar(CP_UTF8, 0, message.c_str(), -1, NULL, 0);
    if (size <= 0) {
        std::cout << message;
        return;
    }
    
    std::wstring wMessage(size - 1, 0);
    MultiByteToWideChar(CP_UTF8, 0, message.c_str(), -1, &wMessage[0], size);
    
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
    WriteConsoleW(hConsole, wMessage.c_str(), static_cast<DWORD>(wMessage.length()), &charsWritten, NULL);
}

/**
 * @brief 辅助函数：将 UTF-8 字符串输出到标准错误
 * @param message UTF-8 编码的消息
 */
static void printUtf8Error(const std::string& message) {
    int size = MultiByteToWideChar(CP_UTF8, 0, message.c_str(), -1, NULL, 0);
    if (size <= 0) {
        std::cerr << message;
        return;
    }
    
    std::wstring wMessage(size - 1, 0);
    MultiByteToWideChar(CP_UTF8, 0, message.c_str(), -1, &wMessage[0], size);
    
    HANDLE hConsole = GetStdHandle(STD_ERROR_HANDLE);
    if (hConsole == INVALID_HANDLE_VALUE || hConsole == NULL) {
        std::cerr << message;
        return;
    }
    
    DWORD mode;
    if (!GetConsoleMode(hConsole, &mode)) {
        std::cerr << message;
        return;
    }
    
    DWORD charsWritten;
    WriteConsoleW(hConsole, wMessage.c_str(), static_cast<DWORD>(wMessage.length()), &charsWritten, NULL);
}
#else
static void printUtf8(const std::string& message) {
    std::cout << message;
}
static void printUtf8Error(const std::string& message) {
    std::cerr << message;
}
#endif

/**
 * @brief 执行外部命令并获取输出
 * @param command 要执行的命令
 * @return 命令的输出结果
 */
std::string CommandHandler::executeCommand(const std::string& command) const {
    LOG_INFO("执行命令: " + command);
    
    std::array<char, 512> buffer = {0};
    std::string result;
    
#ifdef _WIN32
    FILE* pipe = _popen(command.c_str(), "r");
#else
    FILE* pipe = popen(command.c_str(), "r");
#endif
    
    if (!pipe) {
        LOG_ERROR("执行命令失败: " + command);
        return "";
    }
    
    while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
        result += buffer.data();
    }
    
#ifdef _WIN32
    _pclose(pipe);
#else
    pclose(pipe);
#endif
    
    while (!result.empty() && (result.back() == '\n' || result.back() == '\r')) {
        result.pop_back();
    }
    
    return result;
}

/**
 * @brief 获取 Python 可执行文件路径
 * @param pythonPath Python 安装路径
 * @return Python 可执行文件完整路径
 */
std::string CommandHandler::getPythonExecutable(const std::string& pythonPath) const {
    fs::path pythonExe = fs::path(pythonPath) / "python.exe";
    
    if (fs::exists(pythonExe)) {
        LOG_INFO("找到 Python 可执行文件: " + pythonExe.string());
        return pythonExe.string();
    }
    
    fs::path pythonInScripts = fs::path(pythonPath) / "Scripts" / "python.exe";
    if (fs::exists(pythonInScripts)) {
        LOG_INFO("找到 Python 可执行文件: " + pythonInScripts.string());
        return pythonInScripts.string();
    }
    
    LOG_WARNING("未找到 Python 可执行文件在路径: " + pythonPath);
    return "";
}

/**
 * @brief 获取 site-packages 路径
 * @param pythonExe Python 可执行文件路径
 * @return site-packages 路径列表（可能有多个）
 */
std::vector<std::string> CommandHandler::getSitePackagesPaths(const std::string& pythonExe) const {
    std::vector<std::string> paths;
    
    std::string command;
#ifdef _WIN32
    command = "call \"" + pythonExe + "\" -c \"import site; print(chr(10).join(site.getsitepackages()))\"";
#else
    command = "\"" + pythonExe + "\" -c \"import site; print(chr(10).join(site.getsitepackages()))\"";
#endif
    std::string output = executeCommand(command);
    
    if (output.empty()) {
        LOG_WARNING("无法获取 site-packages 路径");
        return paths;
    }
    
    std::istringstream stream(output);
    std::string line;
    while (std::getline(stream, line)) {
        line = trim(line);
        if (!line.empty()) {
            paths.push_back(line);
            LOG_INFO("找到 site-packages 路径: " + line);
        }
    }
    
    return paths;
}

/**
 * @brief 在目录中搜索包
 * @param sitePackagesPath site-packages 路径
 * @param packageName 包名
 * @return 包的完整路径，如果未找到则返回空字符串
 */
std::string CommandHandler::findPackageInPath(const std::string& sitePackagesPath, const std::string& packageName) const {
    try {
        fs::path sitePath(sitePackagesPath);
        
        if (!fs::exists(sitePath) || !fs::is_directory(sitePath)) {
            LOG_WARNING("site-packages 路径不存在或不是目录: " + sitePackagesPath);
            return "";
    }
    
    std::string packageNameLower = packageName;
    std::transform(packageNameLower.begin(), packageNameLower.end(), packageNameLower.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    
    for (const auto& entry : fs::directory_iterator(sitePath)) {
        std::string dirName = entry.path().filename().string();
        std::string dirNameLower = dirName;
        std::transform(dirNameLower.begin(), dirNameLower.end(), dirNameLower.begin(),
                      [](unsigned char c) { return std::tolower(c); });
        
        if (dirNameLower == packageNameLower) {
            LOG_INFO("找到包目录: " + entry.path().string());
            return entry.path().string();
        }
        
        if (dirNameLower.find(packageNameLower + "-") == 0) {
            std::string possiblePackagePath = (sitePath / packageNameLower).string();
            if (fs::exists(possiblePackagePath)) {
                LOG_INFO("通过 dist-info 找到包: " + possiblePackagePath);
                return possiblePackagePath;
            }
        }
    }
    
    for (const auto& entry : fs::directory_iterator(sitePath)) {
        std::string dirName = entry.path().filename().string();
        std::string dirNameLower = dirName;
        std::transform(dirNameLower.begin(), dirNameLower.end(), dirNameLower.begin(),
                      [](unsigned char c) { return std::tolower(c); });
        
        if (dirNameLower.find(packageNameLower) == 0) {
            if (dirNameLower.find("__pycache__") == std::string::npos) {
                LOG_INFO("模糊匹配找到包: " + entry.path().string());
                return entry.path().string();
            }
        }
    }
        
    } catch (const fs::filesystem_error& e) {
        LOG_ERROR("遍历目录失败: " + std::string(e.what()));
    }
    
    return "";
}

/**
 * @brief 获取包版本
 * @param pythonExe Python 可执行文件路径
 * @param packageName 包名
 * @return 包版本字符串，如果无法获取则返回 "未知"
 */
std::string CommandHandler::getPackageVersion(const std::string& pythonExe, const std::string& packageName) const {
    std::string command;
#ifdef _WIN32
    command = "call \"" + pythonExe + "\" -c \"import " + packageName + "; print(" + packageName + ".__version__)\" 2>nul";
#else
    command = "\"" + pythonExe + "\" -c \"import " + packageName + "; print(" + packageName + ".__version__)\" 2>/dev/null";
#endif
    std::string version = executeCommand(command);
    
    if (!version.empty() && version.find("Error") == std::string::npos && version.find("error") == std::string::npos) {
        LOG_INFO("获取包版本成功: " + packageName + " -> " + version);
        return version;
    }
    
#ifdef _WIN32
    command = "call \"" + pythonExe + "\" -c \"import " + packageName + "; print(" + packageName + ".VERSION)\" 2>nul";
#else
    command = "\"" + pythonExe + "\" -c \"import " + packageName + "; print(" + packageName + ".VERSION)\" 2>/dev/null";
#endif
    version = executeCommand(command);
    
    if (!version.empty() && version.find("Error") == std::string::npos && version.find("error") == std::string::npos) {
        LOG_INFO("获取包版本成功: " + packageName + " -> " + version);
        return version;
    }
    
#ifdef _WIN32
    command = "call \"" + pythonExe + "\" -m pip show " + packageName + " 2>nul";
#else
    command = "\"" + pythonExe + "\" -m pip show " + packageName + " 2>/dev/null";
#endif
    std::string pipOutput = executeCommand(command);
    
    std::istringstream stream(pipOutput);
    std::string line;
    while (std::getline(stream, line)) {
        if (line.find("Version:") == 0 || line.find("version:") == 0) {
            size_t colonPos = line.find(':');
            if (colonPos != std::string::npos) {
                std::string ver = line.substr(colonPos + 1);
                ver = trim(ver);
                if (!ver.empty()) {
                    LOG_INFO("通过 pip show 获取包版本: " + packageName + " -> " + ver);
                    return ver;
                }
            }
        }
    }
    
    LOG_WARNING("无法获取包版本: " + packageName);
    return "未知";
}

/**
 * @brief 去除字符串首尾空白字符
 * @param str 输入字符串
 * @return 去除空白后的字符串
 */
std::string CommandHandler::trim(const std::string& str) const {
    size_t start = str.find_first_not_of(" \t\n\r\f\v");
    if (start == std::string::npos) {
        return "";
    }
    size_t end = str.find_last_not_of(" \t\n\r\f\v");
    return str.substr(start, end - start + 1);
}

/**
 * @brief 构造函数
 * @param configManager 配置管理器引用
 */
CommandHandler::CommandHandler(ConfigManager& configManager)
    : configManager_(configManager) {
}

/**
 * @brief 解析并执行命令
 * @param argc 命令行参数数量
 * @param argv 命令行参数数组
 * @return 命令执行结果
 */
CommandResult CommandHandler::parseAndExecute(int argc, char* argv[]) {
    LOG_INFO("开始解析命令行参数，参数数量: " + std::to_string(argc));

    if (argc <= 1) {
        handleHelp();
        return CommandResult::EXIT;
    }

    std::string command = argv[1];
    LOG_INFO("接收到命令: " + command);

    if (isHelpOption(command)) {
        handleHelp();
        return CommandResult::EXIT;
    }

    if (isVersionOption(command)) {
        handleVersion();
        return CommandResult::EXIT;
    }

    if (command == "list") {
        handleList();
        return CommandResult::SUCCESS;
    }
    else if (command == "switch") {
        if (argc < 3) {
            std::string error = "错误: switch 命令需要指定版本号";
            LOG_ERROR(error);
            showErrorAndHelp(error);
            return CommandResult::ERROR;
        }
        handleSwitch(argv[2]);
        return CommandResult::SUCCESS;
    }
    else if (command == "info") {
        handleInfo();
        return CommandResult::SUCCESS;
    }
    else if (command == "gui") {
        handleGui();
        return CommandResult::SUCCESS;
    }
    else if (command == "find") {
        if (argc < 3) {
            std::string error = "错误: find 命令需要指定包名";
            LOG_ERROR(error);
            showErrorAndHelp(error);
            return CommandResult::ERROR;
        }
        handleFind(argv[2]);
        return CommandResult::SUCCESS;
    }
    else if (command == "add") {
        if (argc >= 3) {
            handleAdd(argv[2]);
        } else {
            handleAdd();
        }
        return CommandResult::SUCCESS;
    }
    else if (command == "python") {
        std::vector<std::string> args;
        for (int i = 2; i < argc; ++i) {
            args.push_back(argv[i]);
        }
        handlePython(args);
        return CommandResult::SUCCESS;
    }
    else if (command == "pythonw") {
        std::vector<std::string> args;
        for (int i = 2; i < argc; ++i) {
            args.push_back(argv[i]);
        }
        handlePythonw(args);
        return CommandResult::SUCCESS;
    }
    else if (command == "pip") {
        std::vector<std::string> args;
        for (int i = 2; i < argc; ++i) {
            args.push_back(argv[i]);
        }
        handlePip(args);
        return CommandResult::SUCCESS;
    }
    else {
        std::string error = "错误: 未知命令 '" + command + "'";
        LOG_ERROR(error);
        showErrorAndHelp(error);
        return CommandResult::ERROR;
    }
}

/**
 * @brief 显示帮助信息
 */
void CommandHandler::handleHelp() {
    printUtf8("Python 版本管理器 v" + std::string(VERSION) + "\n\n");
    printUtf8("用法: pym <command> [arguments]\n\n");
    printUtf8("命令:\n");
    printUtf8("  --help, -h         显示帮助信息\n");
    printUtf8("  --version, -v      显示版本号\n");
    printUtf8("  list               列出已安装的 Python 版本\n");
    printUtf8("  switch <version>   切换到指定 Python 版本\n");
    printUtf8("  add [path]         添加 Python 版本（可选路径参数）\n");
    printUtf8("  info               查看当前 Python 版本信息\n");
    printUtf8("  gui                启动 GUI 界面\n");
    printUtf8("  find <package>     查找已安装的 Python 包\n");
    printUtf8("  python <command>   执行 Python 命令（显示详细日志）\n");
    printUtf8("  pythonw <command>  执行 Pythonw 命令（显示详细日志）\n");
    printUtf8("  pip <command>      执行 pip 命令（显示详细日志）\n\n");
    printUtf8("示例:\n");
    printUtf8("  pym list\n");
    printUtf8("  pym switch 3.10.10\n");
    printUtf8("  pym add E:\\environment\\python\\python311\n");
    printUtf8("  pym add\n");
    printUtf8("  pym info\n");
    printUtf8("  pym gui\n");
    printUtf8("  pym find numpy\n");
    printUtf8("  pym python --version\n");
    printUtf8("  pym python -c \"print('Hello')\"\n");
    printUtf8("  pym pip list\n");

    LOG_INFO("显示帮助信息");
}

/**
 * @brief 显示版本信息
 */
void CommandHandler::handleVersion() {
    printUtf8("pym 版本 " + std::string(VERSION) + "\n");
    LOG_INFO("显示版本信息: " + std::string(VERSION));
}

/**
 * @brief 处理 list 命令 - 列出已安装的 Python 版本
 */
void CommandHandler::handleList() {
    LOG_INFO("执行 list 命令");
    
    printUtf8("已安装的 Python 版本:\n\n");
    
    try {
        auto paths = configManager_.getAllPythonPaths();
        std::string currentVersion = configManager_.getCurrentVersion();
        
        if (paths.empty()) {
            printUtf8("  (无已安装的 Python 版本)\n");
        } else {
            for (const auto& [version, path] : paths) {
                if (version == currentVersion) {
                    printUtf8("  * " + version + " (当前)\n");
                    printUtf8("    路径: " + path + "\n");
                } else {
                    printUtf8("    " + version + "\n");
                    printUtf8("    路径: " + path + "\n");
                }
            }
        }
    } catch (const std::exception& e) {
        LOG_ERROR("获取 Python 版本列表失败: " + std::string(e.what()));
        printUtf8("  获取版本列表失败: " + std::string(e.what()) + "\n");
    }
}

/**
 * @brief 处理 switch 命令 - 切换 Python 版本
 * @param version 目标版本
 */
void CommandHandler::handleSwitch(const std::string& version) {
    LOG_INFO("执行 switch 命令，目标版本: " + version);
    
    printUtf8("切换 Python 版本到 " + version + "...\n\n");
    
    try {
        if (!configManager_.hasPythonVersion(version)) {
            printUtf8("错误: Python 版本 " + version + " 未安装\n");
            printUtf8("请使用 'pym list' 查看已安装的版本\n");
            LOG_WARNING("尝试切换到未安装的版本: " + version);
            return;
        }
        
        configManager_.setCurrentVersion(version);
        configManager_.saveConfig();

        printUtf8("成功切换到 Python " + version + "\n");
        printUtf8("路径: " + configManager_.getPythonPath(version) + "\n");
        LOG_INFO("成功切换到 Python " + version);
        
    } catch (const ConfigException& e) {
        LOG_ERROR("切换版本失败: " + std::string(e.what()));
        printUtf8("切换版本失败: " + std::string(e.what()) + "\n");
    }
}

/**
 * @brief 处理 info 命令 - 查看当前 Python 版本信息
 * 
 * 该方法执行以下步骤：
 * 1. 从配置文件获取当前 Python 版本
 * 2. 获取 Python 可执行文件路径
 * 3. 执行 python --version 获取版本号
 * 4. 执行 python -c "import sys; print(sys.version)" 获取详细版本信息
 * 5. 执行 python -c "import platform; print(platform.platform())" 获取平台信息
 * 6. 格式化输出所有信息
 */
void CommandHandler::handleInfo() {
    LOG_INFO("执行 info 命令 - 查看当前 Python 版本信息");
    
    try {
        std::string currentVersion = configManager_.getCurrentVersion();
        if (currentVersion.empty()) {
            printUtf8("错误: 未配置当前 Python 版本\n");
            printUtf8("请先使用 'pym switch <version>' 设置当前版本\n");
            LOG_ERROR("未配置当前 Python 版本");
            return;
        }
        LOG_INFO("当前配置的 Python 版本: " + currentVersion);
        
        if (!configManager_.hasPythonVersion(currentVersion)) {
            printUtf8("错误: 当前 Python 版本 " + currentVersion + " 的路径未配置\n");
            printUtf8("请检查 config.json 配置文件\n");
            LOG_ERROR("Python 版本路径未配置: " + currentVersion);
            return;
        }
        
        std::string pythonPath = configManager_.getPythonPath(currentVersion);
        LOG_INFO("Python 安装路径: " + pythonPath);
        
        std::string pythonExe = getPythonExecutable(pythonPath);
        if (pythonExe.empty()) {
            printUtf8("错误: 无法找到 Python 可执行文件\n");
            printUtf8("请检查配置的路径是否正确: " + pythonPath + "\n");
            LOG_ERROR("无法找到 Python 可执行文件");
            return;
        }
        
        if (!fs::exists(pythonExe)) {
            printUtf8("错误: Python 可执行文件不存在\n");
            printUtf8("路径: " + pythonExe + "\n");
            LOG_ERROR("Python 可执行文件不存在: " + pythonExe);
            return;
        }
        
        std::string versionCommand;
#ifdef _WIN32
        versionCommand = "call \"" + pythonExe + "\" --version";
#else
        versionCommand = "\"" + pythonExe + "\" --version";
#endif
        std::string versionOutput = executeCommand(versionCommand);
        LOG_INFO("Python --version 输出: " + versionOutput);

        std::string detailedVersionCommand;
#ifdef _WIN32
        detailedVersionCommand = "call \"" + pythonExe + "\" -c \"import sys; print(sys.version)\"";
#else
        detailedVersionCommand = "\"" + pythonExe + "\" -c \"import sys; print(sys.version)\"";
#endif
        std::string detailedVersion = executeCommand(detailedVersionCommand);
        LOG_INFO("Python 详细版本信息已获取");

        std::string platformCommand;
#ifdef _WIN32
        platformCommand = "call \"" + pythonExe + "\" -c \"import platform; print(platform.platform())\"";
#else
        platformCommand = "\"" + pythonExe + "\" -c \"import platform; print(platform.platform())\"";
#endif
        std::string platformInfo = executeCommand(platformCommand);
        LOG_INFO("平台信息已获取: " + platformInfo);
        
        printUtf8("\n");
        printUtf8("当前 Python 版本: " + currentVersion + "\n");
        printUtf8("\n");
        printUtf8("Python 可执行文件:\n");
        printUtf8("  " + pythonExe + "\n");
        printUtf8("\n");
        
        if (!versionOutput.empty()) {
            printUtf8("版本信息:\n");
            printUtf8("  " + versionOutput + "\n");
            printUtf8("\n");
        }
        
        if (!detailedVersion.empty()) {
            printUtf8("详细版本信息:\n");
            std::istringstream stream(detailedVersion);
            std::string line;
            while (std::getline(stream, line)) {
                line = trim(line);
                if (!line.empty()) {
                    printUtf8("  " + line + "\n");
                }
            }
            printUtf8("\n");
        }
        
        if (!platformInfo.empty()) {
            printUtf8("平台信息:\n");
            printUtf8("  " + platformInfo + "\n");
            printUtf8("\n");
        }
        
        LOG_INFO("info 命令执行成功");
        
    } catch (const ConfigException& e) {
        LOG_ERROR("获取版本信息失败: " + std::string(e.what()));
        printUtf8("\n错误: " + std::string(e.what()) + "\n");
        printUtf8("请检查 config.json 配置文件是否正确\n");
    } catch (const fs::filesystem_error& e) {
        LOG_ERROR("文件系统错误: " + std::string(e.what()));
        printUtf8("\n文件系统错误: " + std::string(e.what()) + "\n");
    } catch (const std::exception& e) {
        LOG_ERROR("获取版本信息时发生未知错误: " + std::string(e.what()));
        printUtf8("\n未知错误: " + std::string(e.what()) + "\n");
    }
}

/**
 * @brief 处理 gui 命令 - 启动 GUI 界面
 */
void CommandHandler::handleGui() {
    LOG_INFO("执行 gui 命令");
    
    printUtf8("启动 GUI 界面...\n\n");
    printUtf8("[占位功能] GUI 界面将在后续版本实现\n");
    printUtf8("提示: 当前仅支持命令行模式\n");
}

/**
 * @brief 处理 find 命令 - 查找已安装的 Python 包
 * @param packageName 包名
 *
 * 该方法执行以下步骤：
 * 1. 从配置文件获取当前 Python 版本和路径
 * 2. 获取 Python 可执行文件路径
 * 3. 执行 Python 命令获取 site-packages 路径
 * 4. 在 site-packages 目录中搜索指定的包
 * 5. 如果找到包，显示包信息并询问用户是否添加到配置文件
 */
void CommandHandler::handleFind(const std::string& packageName) {
    LOG_INFO("执行 find 命令，查找包: " + packageName);

    std::cout << "搜索包: " << packageName << "\n\n";

    try {
        std::string currentVersion = configManager_.getCurrentVersion();
        if (currentVersion.empty()) {
            std::cout << "错误: 未配置当前 Python 版本\n";
            std::cout << "请先使用 'pym switch <version>' 设置当前版本\n";
            LOG_ERROR("未配置当前 Python 版本");
            return;
        }
        LOG_INFO("当前 Python 版本: " + currentVersion);

        if (!configManager_.hasPythonVersion(currentVersion)) {
            std::cout << "错误: 当前 Python 版本 " << currentVersion << " 的路径未配置\n";
            LOG_ERROR("Python 版本路径未配置: " + currentVersion);
            return;
        }

        std::string pythonPath = configManager_.getPythonPath(currentVersion);
        LOG_INFO("Python 安装路径: " + pythonPath);

        std::string pythonExe = getPythonExecutable(pythonPath);
        if (pythonExe.empty()) {
            std::cout << "错误: 无法找到 Python 可执行文件\n";
            std::cout << "请检查配置的路径是否正确: " << pythonPath << "\n";
            LOG_ERROR("无法找到 Python 可执行文件");
            return;
        }

        std::vector<std::string> sitePackagesPaths = getSitePackagesPaths(pythonExe);
        if (sitePackagesPaths.empty()) {
            std::cout << "错误: 无法获取 site-packages 路径\n";
            LOG_ERROR("无法获取 site-packages 路径");
            return;
        }

        std::string packagePath;
        for (const auto& sitePath : sitePackagesPaths) {
            packagePath = findPackageInPath(sitePath, packageName);
            if (!packagePath.empty()) {
                break;
            }
        }

        if (packagePath.empty()) {
            std::cout << "未找到包: " << packageName << "\n\n";
            std::cout << "提示: 该包可能未安装，请使用以下命令安装:\n";
            std::cout << "  pip install " << packageName << "\n";
            LOG_WARNING("未找到包: " + packageName);
            return;
        }

        std::string packageVersion = getPackageVersion(pythonExe, packageName);

        std::cout << "找到包:\n";
        std::cout << "  名称: " << packageName << "\n";
        std::cout << "  版本: " << packageVersion << "\n";
        std::cout << "  路径: " << packagePath << "\n\n";

        LOG_INFO("找到包: " + packageName + ", 版本: " + packageVersion + ", 路径: " + packagePath);

        std::cout << "是否添加到配置文件? (y/n): ";
        std::string input;
        std::getline(std::cin, input);

        input = trim(input);
        if (input == "y" || input == "Y" || input == "yes" || input == "YES") {
            PackageInfo packageInfo;
            packageInfo.name = packageName;
            packageInfo.version = packageVersion;
            packageInfo.pythonVersion = currentVersion;

            configManager_.addPackage(packageInfo);
            configManager_.saveConfig();

            std::cout << "\n包信息已添加到配置文件\n";
            LOG_INFO("包信息已添加到配置文件: " + packageName);
        } else {
            std::cout << "\n已取消添加\n";
            LOG_INFO("用户取消添加包到配置文件: " + packageName);
        }

    } catch (const ConfigException& e) {
        LOG_ERROR("查找包失败: " + std::string(e.what()));
        std::cout << "查找包失败: " << e.what() << "\n";
    } catch (const std::exception& e) {
        LOG_ERROR("查找包时发生未知错误: " + std::string(e.what()));
        std::cout << "查找包时发生未知错误: " << e.what() << "\n";
    }
}

/**
 * @brief 处理 add 命令 - 添加 Python 版本
 * @param path Python 安装路径（可选，如果为空则打开文件夹选择对话框）
 *
 * 该方法执行以下步骤：
 * 1. 如果没有提供路径，打开文件夹选择对话框
 * 2. 验证路径是否存在
 * 3. 验证路径是否包含 python.exe
 * 4. 获取 Python 版本号
 * 5. 检查版本号是否已存在，如果存在则询问是否覆盖
 * 6. 添加到配置文件
 * 7. 询问用户是否切换到此版本
 */
void CommandHandler::handleAdd(const std::string& path) {
    LOG_INFO("执行 add 命令");

    std::string selectedPath = path;

    // 步骤1：如果没有提供路径，打开文件夹选择对话框
    if (selectedPath.empty()) {
#ifdef _WIN32
        // 使用 Windows API 打开文件夹选择对话框
        HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
        if (SUCCEEDED(hr)) {
            IFileOpenDialog* pFileOpen = NULL;

            // 创建文件打开对话框
            hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL,
                                   IID_IFileOpenDialog, reinterpret_cast<void**>(&pFileOpen));

            if (SUCCEEDED(hr)) {
            DWORD dwOptions;
            hr = pFileOpen->GetOptions(&dwOptions);
            if (SUCCEEDED(hr)) {
                hr = pFileOpen->SetOptions(dwOptions | FOS_PICKFOLDERS);
            }

            pFileOpen->SetTitle(L"选择 Python 安装目录");

            hr = pFileOpen->Show(NULL);

            if (SUCCEEDED(hr)) {
                IShellItem* pItem;
                hr = pFileOpen->GetResult(&pItem);
                if (SUCCEEDED(hr)) {
                    LPWSTR pszFilePath;
                    hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);
                    if (SUCCEEDED(hr)) {
                        int size_needed = WideCharToMultiByte(CP_UTF8, 0, pszFilePath, -1, NULL, 0, NULL, NULL);
                        std::string result(size_needed, 0);
                        WideCharToMultiByte(CP_UTF8, 0, pszFilePath, -1, &result[0], size_needed, NULL, NULL);
                        selectedPath = result;
                        if (!selectedPath.empty() && selectedPath.back() == '\0') {
                            selectedPath.pop_back();
                        }
                        CoTaskMemFree(pszFilePath);
                    }
                    pItem->Release();
                }
            }

            pFileOpen->Release();
        }

        CoUninitialize();
    }

    if (selectedPath.empty()) {
        std::cout << "已取消选择\n";
        LOG_INFO("用户取消选择文件夹");
        return;
    }
#else
    std::cout << "错误: 非Windows平台暂不支持文件夹选择对话框\n";
    std::cout << "请使用命令: pym add <path>\n";
    LOG_ERROR("非Windows平台不支持文件夹选择对话框");
    return;
#endif
}

    std::cout << "\n添加 Python 版本...\n\n";
    std::cout << "路径: " << selectedPath << "\n";

    if (!fs::exists(selectedPath)) {
        std::cout << "错误: 路径不存在\n";
        LOG_ERROR("路径不存在: " + selectedPath);
        return;
    }

    if (!fs::is_directory(selectedPath)) {
        std::cout << "错误: 路径不是目录\n";
        LOG_ERROR("路径不是目录: " + selectedPath);
        return;
    }

    fs::path pythonExePath = fs::path(selectedPath) / "python.exe";
    if (!fs::exists(pythonExePath)) {
        std::cout << "错误: 路径不包含 python.exe，不是有效的 Python 安装目录\n";
        LOG_ERROR("路径不包含 python.exe: " + selectedPath);
        return;
    }

    std::string versionCommand;
#ifdef _WIN32
    versionCommand = "call \"" + pythonExePath.string() + "\" --version";
#else
    versionCommand = "\"" + pythonExePath.string() + "\" --version";
#endif
    std::string versionOutput = executeCommand(versionCommand);
    LOG_INFO("Python --version 输出: " + versionOutput);

    if (versionOutput.empty()) {
        std::cout << "错误: 无法获取 Python 版本号\n";
        LOG_ERROR("无法获取 Python 版本号");
        return;
    }

    std::string pythonVersion;
    size_t spacePos = versionOutput.find(' ');
    if (spacePos != std::string::npos) {
        pythonVersion = versionOutput.substr(spacePos + 1);
        pythonVersion = trim(pythonVersion);
    } else {
        pythonVersion = versionOutput;
    }

    if (pythonVersion.empty()) {
        std::cout << "错误: 无法解析 Python 版本号\n";
        LOG_ERROR("无法解析 Python 版本号: " + versionOutput);
        return;
    }

    std::cout << "检测到 Python 版本: " << pythonVersion << "\n\n";

    if (configManager_.hasPythonVersion(pythonVersion)) {
        std::string existingPath = configManager_.getPythonPath(pythonVersion);
        std::cout << "警告: Python " << pythonVersion << " 已存在\n";
        std::cout << "现有路径: " << existingPath << "\n\n";
        std::cout << "是否覆盖? (y/n): ";

        std::string input;
        std::getline(std::cin, input);
        input = trim(input);

        if (input != "y" && input != "Y" && input != "yes" && input != "YES") {
            std::cout << "\n已取消添加\n";
            LOG_INFO("用户取消覆盖现有版本: " + pythonVersion);
            return;
        }
    }

    try {
        configManager_.addPythonPath(pythonVersion, selectedPath);
        configManager_.saveConfig();

        std::cout << "成功添加 Python " << pythonVersion << "\n";
        std::cout << "路径: " << selectedPath << "\n\n";
        LOG_INFO("成功添加 Python 版本: " + pythonVersion + ", 路径: " + selectedPath);

        std::cout << "是否切换到此版本? (y/n): ";
        std::string input;
        std::getline(std::cin, input);
        input = trim(input);

        if (input == "y" || input == "Y" || input == "yes" || input == "YES") {
            configManager_.setCurrentVersion(pythonVersion);
            configManager_.saveConfig();
            std::cout << "已切换到 Python " << pythonVersion << "\n";
            LOG_INFO("已切换到 Python " + pythonVersion);
        }

    } catch (const ConfigException& e) {
        LOG_ERROR("添加 Python 版本失败: " + std::string(e.what()));
        std::cout << "添加失败: " << e.what() << "\n";
    } catch (const std::exception& e) {
        LOG_ERROR("添加 Python 版本时发生未知错误: " + std::string(e.what()));
        std::cout << "添加时发生未知错误: " << e.what() << "\n";
    }
}

/**
 * @brief 显示错误消息和帮助信息
 * @param errorMessage 错误消息
 */
void CommandHandler::showErrorAndHelp(const std::string& errorMessage) {
    printUtf8Error(errorMessage + "\n\n");
    handleHelp();
}

/**
 * @brief 检查是否为帮助选项
 * @param arg 参数
 * @return 是否为帮助选项
 */
bool CommandHandler::isHelpOption(const std::string& arg) const {
    return arg == "--help" || arg == "-h";
}

/**
 * @brief 检查是否为版本选项
 * @param arg 参数
 * @return 是否为版本选项
 */
bool CommandHandler::isVersionOption(const std::string& arg) const {
    return arg == "--version" || arg == "-v";
}

/**
 * @brief 获取可执行文件所在目录
 * @return 可执行文件所在目录的路径
 */
std::string CommandHandler::getExecutableDirectory() const {
#ifdef _WIN32
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
#else
    return "";
#endif
}

/**
 * @brief 执行进程并等待其完成（设置环境变量）
 * @param executablePath 可执行文件路径
 * @param args 命令行参数
 * @param verbose 是否启用详细日志输出
 * @return 进程退出码
 */
int CommandHandler::executeProcessWithEnv(const std::string& executablePath, const std::vector<std::string>& args, bool verbose) {
#ifdef _WIN32
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    si.hStdError = GetStdHandle(STD_ERROR_HANDLE);
    si.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    si.dwFlags |= STARTF_USESTDHANDLES;

    std::string fullCommandLine = "\"" + executablePath + "\"";
    for (const auto& arg : args) {
        fullCommandLine += " ";
        if (arg.find(' ') != std::string::npos) {
            fullCommandLine += "\"" + arg + "\"";
        } else {
            fullCommandLine += arg;
        }
    }
    
    if (fullCommandLine.length() > 32767) {
        LOG_ERROR("命令行过长，超过最大限制");
        return -1;
    }
    
    std::vector<char> cmdLineBuffer(fullCommandLine.begin(), fullCommandLine.end());
    cmdLineBuffer.push_back('\0');

    LOG_INFO("执行命令: " + fullCommandLine);

    std::string envString;
    if (verbose) {
        LPWCH currentEnv = GetEnvironmentStringsW();
        if (currentEnv) {
            LPWCH envPtr = currentEnv;
            while (*envPtr) {
                int size_needed = WideCharToMultiByte(CP_UTF8, 0, envPtr, -1, NULL, 0, NULL, NULL);
                if (size_needed > 0) {
                    std::string envVar(size_needed - 1, 0);
                    WideCharToMultiByte(CP_UTF8, 0, envPtr, -1, &envVar[0], size_needed, NULL, NULL);
                    envString += envVar + '\0';
                }
                envPtr += wcslen(envPtr) + 1;
            }
            FreeEnvironmentStringsW(currentEnv);
        }
        envString += "PYM_VERBOSE=1\0";
        envString += '\0';
    }

    BOOL success = CreateProcessA(
        NULL,
        cmdLineBuffer.data(),
        NULL,
        NULL,
        TRUE,
        0,
        verbose ? (LPVOID)envString.c_str() : NULL,
        NULL,
        &si,
        &pi
    );

    if (!success) {
        DWORD error = GetLastError();
        LOG_ERROR("无法创建进程，错误代码: " + std::to_string(error));
        return -1;
    }

    CloseHandle(pi.hThread);

    DWORD waitResult = WaitForSingleObject(pi.hProcess, 5000);
    
    if (waitResult == WAIT_TIMEOUT) {
        LOG_WARNING("等待进程结束超时（5秒），进程可能仍在运行");
    }

    DWORD exitCode = 0;
    if (!GetExitCodeProcess(pi.hProcess, &exitCode)) {
        exitCode = 1;
        LOG_ERROR("无法获取子进程退出码");
    }

    CloseHandle(pi.hProcess);

    LOG_INFO("子进程退出码: " + std::to_string(exitCode));
    return static_cast<int>(exitCode);
#else
    // 非 Windows 系统的简单实现
    std::string command = "\"" + executablePath + "\"";
    for (const auto& arg : args) {
        command += " " + arg;
    }
    return system(command.c_str());
#endif
}

/**
 * @brief 处理 python 命令 - 执行 Python 命令
 * @param args 命令行参数
 */
void CommandHandler::handlePython(const std::vector<std::string>& args) {
    LOG_INFO("执行 python 命令");
    
    std::string exeDir = getExecutableDirectory();
    if (exeDir.empty()) {
        printUtf8Error("错误: 无法获取可执行文件目录\n");
        LOG_ERROR("无法获取可执行文件目录");
        return;
    }
    
    std::string pythonProxyPath = exeDir + "\\python.exe";
    
    if (!fs::exists(pythonProxyPath)) {
        printUtf8Error("错误: Python 代理程序不存在: " + pythonProxyPath + "\n");
        LOG_ERROR("Python 代理程序不存在: " + pythonProxyPath);
        return;
    }
    
    LOG_INFO("Python 代理程序路径: " + pythonProxyPath);
    
    int exitCode = executeProcessWithEnv(pythonProxyPath, args, true);
    
    LOG_INFO("Python 命令执行完成，退出码: " + std::to_string(exitCode));
}

/**
 * @brief 处理 pythonw 命令 - 执行 Pythonw 命令
 * @param args 命令行参数
 */
void CommandHandler::handlePythonw(const std::vector<std::string>& args) {
    LOG_INFO("执行 pythonw 命令");
    
    std::string exeDir = getExecutableDirectory();
    if (exeDir.empty()) {
        printUtf8Error("错误: 无法获取可执行文件目录\n");
        LOG_ERROR("无法获取可执行文件目录");
        return;
    }
    
    std::string pythonwProxyPath = exeDir + "\\pythonw.exe";
    
    if (!fs::exists(pythonwProxyPath)) {
        printUtf8Error("错误: Pythonw 代理程序不存在: " + pythonwProxyPath + "\n");
        LOG_ERROR("Pythonw 代理程序不存在: " + pythonwProxyPath);
        return;
    }
    
    LOG_INFO("Pythonw 代理程序路径: " + pythonwProxyPath);
    
    int exitCode = executeProcessWithEnv(pythonwProxyPath, args, true);
    
    LOG_INFO("Pythonw 命令执行完成，退出码: " + std::to_string(exitCode));
}

/**
 * @brief 处理 pip 命令 - 执行 pip 命令
 * @param args 命令行参数
 */
void CommandHandler::handlePip(const std::vector<std::string>& args) {
    LOG_INFO("执行 pip 命令");
    
    std::string exeDir = getExecutableDirectory();
    if (exeDir.empty()) {
        printUtf8Error("错误: 无法获取可执行文件目录\n");
        LOG_ERROR("无法获取可执行文件目录");
        return;
    }
    
    std::string pipProxyPath = exeDir + "\\pip.exe";
    
    if (!fs::exists(pipProxyPath)) {
        printUtf8Error("错误: pip 代理程序不存在: " + pipProxyPath + "\n");
        LOG_ERROR("pip 代理程序不存在: " + pipProxyPath);
        return;
    }
    
    LOG_INFO("pip 代理程序路径: " + pipProxyPath);
    
    int exitCode = executeProcessWithEnv(pipProxyPath, args, true);
    
    LOG_INFO("pip 命令执行完成，退出码: " + std::to_string(exitCode));
}