#ifndef COMMAND_HANDLER_H
#define COMMAND_HANDLER_H

#include <string>
#include <vector>
#include <memory>
#include "config_manager.h"
#include "logger.h"

/**
 * @brief 命令处理结果枚举
 */
enum class CommandResult {
    SUCCESS,        // 命令执行成功
    ERROR,          // 命令执行出错
    EXIT           // 需要退出程序（如 --help 或 --version）
};

/**
 * @brief 命令处理器类
 * 
 * 负责解析和处理 pym 命令行工具的各种命令
 */
class CommandHandler {
public:
    /**
     * @brief 构造函数
     * @param configManager 配置管理器引用
     */
    explicit CommandHandler(ConfigManager& configManager);

    /**
     * @brief 析构函数
     */
    ~CommandHandler() = default;

    // 禁止拷贝
    CommandHandler(const CommandHandler&) = delete;
    CommandHandler& operator=(const CommandHandler&) = delete;

    // 允许移动
    CommandHandler(CommandHandler&&) noexcept = default;
    CommandHandler& operator=(CommandHandler&&) noexcept = default;

    /**
     * @brief 解析并执行命令
     * @param argc 命令行参数数量
     * @param argv 命令行参数数组
     * @return 命令执行结果
     */
    CommandResult parseAndExecute(int argc, char* argv[]);

    /**
     * @brief 显示帮助信息
     */
    void handleHelp();

    /**
     * @brief 显示版本信息
     */
    void handleVersion();

    /**
     * @brief 处理 list 命令 - 列出已安装的 Python 版本
     */
    void handleList();

    /**
     * @brief 处理 switch 命令 - 切换 Python 版本
     * @param version 目标版本
     */
    void handleSwitch(const std::string& version);

    /**
     * @brief 处理 info 命令 - 查看当前 Python 版本信息
     */
    void handleInfo();

    /**
     * @brief 处理 gui 命令 - 启动 GUI 界面
     */
    void handleGui();

    /**
     * @brief 处理 find 命令 - 查找已安装的 Python 包
     * @param packageName 包名
     */
    void handleFind(const std::string& packageName);

    /**
     * @brief 处理 add 命令 - 添加 Python 版本
     * @param path Python 安装路径（可选，如果为空则打开文件夹选择对话框）
     */
    void handleAdd(const std::string& path = "");

    /**
     * @brief 处理 python 命令 - 执行 Python 命令
     * @param args 命令行参数
     */
    void handlePython(const std::vector<std::string>& args);

    /**
     * @brief 处理 pythonw 命令 - 执行 Pythonw 命令
     * @param args 命令行参数
     */
    void handlePythonw(const std::vector<std::string>& args);

    /**
     * @brief 处理 pip 命令 - 执行 pip 命令
     * @param args 命令行参数
     */
    void handlePip(const std::vector<std::string>& args);

private:
    /**
     * @brief 显示错误消息和帮助信息
     * @param errorMessage 错误消息
     */
    void showErrorAndHelp(const std::string& errorMessage);

    /**
     * @brief 检查是否为帮助选项
     * @param arg 参数
     * @return 是否为帮助选项
     */
    bool isHelpOption(const std::string& arg) const;

    /**
     * @brief 检查是否为版本选项
     * @param arg 参数
     * @return 是否为版本选项
     */
    bool isVersionOption(const std::string& arg) const;

    /**
     * @brief 执行外部命令并获取输出
     * @param command 要执行的命令
     * @return 命令的输出结果
     */
    std::string executeCommand(const std::string& command) const;

    /**
     * @brief 获取 Python 可执行文件路径
     * @param pythonPath Python 安装路径
     * @return Python 可执行文件完整路径
     */
    std::string getPythonExecutable(const std::string& pythonPath) const;

    /**
     * @brief 获取 site-packages 路径
     * @param pythonExe Python 可执行文件路径
     * @return site-packages 路径列表（可能有多个）
     */
    std::vector<std::string> getSitePackagesPaths(const std::string& pythonExe) const;

    /**
     * @brief 在目录中搜索包
     * @param sitePackagesPath site-packages 路径
     * @param packageName 包名
     * @return 包的完整路径，如果未找到则返回空字符串
     */
    std::string findPackageInPath(const std::string& sitePackagesPath, const std::string& packageName) const;

    /**
     * @brief 获取包版本
     * @param pythonExe Python 可执行文件路径
     * @param packageName 包名
     * @return 包版本字符串，如果无法获取则返回 "未知"
     */
    std::string getPackageVersion(const std::string& pythonExe, const std::string& packageName) const;

    /**
     * @brief 去除字符串首尾空白字符
     * @param str 输入字符串
     * @return 去除空白后的字符串
     */
    std::string trim(const std::string& str) const;

    /**
     * @brief 获取可执行文件所在目录
     * @return 可执行文件所在目录的路径
     */
    std::string getExecutableDirectory() const;

    /**
     * @brief 执行进程并等待其完成（设置环境变量）
     * @param executablePath 可执行文件路径
     * @param args 命令行参数
     * @param verbose 是否启用详细日志输出
     * @return 进程退出码
     */
    int executeProcessWithEnv(const std::string& executablePath, const std::vector<std::string>& args, bool verbose);

    ConfigManager& configManager_;  // 配置管理器引用
    static constexpr const char* VERSION = "1.3.3";  // 版本号
};

#endif // COMMAND_HANDLER_H