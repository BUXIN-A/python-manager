#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <string>
#include <map>
#include <vector>
#include <memory>
#include <mutex>
#include <chrono>
#include "json.hpp"
#include "logger.h"

/**
 * @brief 包信息结构体
 */
struct PackageInfo {
    std::string name;        // 包名
    std::string version;    // 包版本
    std::string pythonVersion;  // 安装的 Python 版本

    // 转换为 JSON
    nlohmann::json toJson() const;
    // 从 JSON 解析
    static PackageInfo fromJson(const nlohmann::json& j);
};

/**
 * @brief 配置管理器异常类
 */
class ConfigException : public std::exception {
public:
    explicit ConfigException(const std::string& message);
    const char* what() const noexcept override;

private:
    std::string message_;
};

/**
 * @brief 配置管理器类
 * 
 * 负责读取、写入和管理应用程序配置文件
 * 使用 RAII 原则，确保资源正确释放
 * 使用缓存机制提高性能，避免重复读取配置文件
 * 线程安全，支持多线程并发访问
 */
class ConfigManager {
public:
    /**
     * @brief 构造函数
     * @param configPath 配置文件路径，默认为 "config.json"
     */
    explicit ConfigManager(const std::string& configPath = "config.json");

    /**
     * @brief 析构函数
     */
    ~ConfigManager() = default;

    // 禁止拷贝
    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;

    // 允许移动
    ConfigManager(ConfigManager&&) noexcept = default;
    ConfigManager& operator=(ConfigManager&&) noexcept = default;

    /**
     * @brief 加载配置文件
     * @throws ConfigException 当文件不存在或 JSON 解析失败时抛出异常
     */
    void loadConfig();

    /**
     * @brief 保存配置到文件
     * @throws ConfigException 当文件写入失败时抛出异常
     */
    void saveConfig();

    /**
     * @brief 获取当前 Python 版本
     * @return 当前 Python 版本字符串
     */
    std::string getCurrentVersion() const;

    /**
     * @brief 设置当前 Python 版本
     * @param version Python 版本字符串
     * @throws ConfigException 当版本不存在于 python_path 中时抛出异常
     */
    void setCurrentVersion(const std::string& version);

    /**
     * @brief 获取指定版本的 Python 路径
     * @param version Python 版本
     * @return Python 安装路径
     * @throws ConfigException 当版本不存在时抛出异常
     */
    std::string getPythonPath(const std::string& version) const;

    /**
     * @brief 添加新的 Python 版本路径
     * @param version Python 版本
     * @param path 安装路径
     */
    void addPythonPath(const std::string& version, const std::string& path);

    /**
     * @brief 移除 Python 版本路径
     * @param version Python 版本
     * @return 是否成功移除
     */
    bool removePythonPath(const std::string& version);

    /**
     * @brief 获取所有 Python 版本路径
     * @return 版本到路径的映射
     */
    std::map<std::string, std::string> getAllPythonPaths() const;

    /**
     * @brief 获取包列表
     * @return 包信息列表
     */
    std::vector<PackageInfo> getPackages() const;

    /**
     * @brief 添加包信息
     * @param package 包信息
     */
    void addPackage(const PackageInfo& package);

    /**
     * @brief 移除包信息
     * @param packageName 包名
     * @return 是否成功移除
     */
    bool removePackage(const std::string& packageName);

    /**
     * @brief 检查配置是否已加载
     * @return 是否已加载
     */
    bool isLoaded() const;

    /**
     * @brief 获取配置文件路径
     * @return 配置文件路径
     */
    std::string getConfigPath() const;

    /**
     * @brief 重新加载配置文件（刷新缓存）
     * @throws ConfigException 当加载失败时抛出异常
     */
    void reloadConfig();

    /**
     * @brief 清除缓存
     */
    void clearCache();

    /**
     * @brief 检查指定版本的 Python 是否存在
     * @param version Python 版本
     * @return 是否存在
     */
    bool hasPythonVersion(const std::string& version) const;

    /**
     * @brief 检查缓存是否有效
     * @return 缓存是否有效
     */
    bool isCacheValid() const;

    /**
     * @brief 获取缓存时间戳
     * @return 缓存时间戳
     */
    std::chrono::system_clock::time_point getCacheTimestamp() const;

private:
    std::string configPath_;        // 配置文件路径
    nlohmann::json config_;         // 配置 JSON 对象
    bool loaded_;                   // 是否已加载
    
    // 缓存相关成员
    mutable std::mutex mutex_;      // 互斥锁，保证线程安全
    std::chrono::system_clock::time_point cacheTimestamp_;  // 缓存时间戳
    bool cacheValid_;               // 缓存是否有效
    
    // 缓存数据
    mutable std::string cachedCurrentVersion_;  // 缓存的当前版本
    mutable std::map<std::string, std::string> cachedPythonPaths_;  // 缓存的 Python 路径
    mutable std::vector<PackageInfo> cachedPackages_;  // 缓存的包列表
    mutable bool cachePopulated_;   // 缓存是否已填充

    /**
     * @brief 验证配置文件结构
     * @throws ConfigException 当配置结构无效时抛出异常
     */
    void validateConfig() const;

    /**
     * @brief 创建默认配置
     */
    void createDefaultConfig();

    /**
     * @brief 更新缓存
     */
    void updateCache() const;

    /**
     * @brief 检查缓存是否需要更新
     * @return 是否需要更新缓存
     */
    bool needsCacheUpdate() const;
};

#endif // CONFIG_MANAGER_H