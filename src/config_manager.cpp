#include "config_manager.h"
#include <fstream>
#include <sstream>
#include <filesystem>

nlohmann::json PackageInfo::toJson() const {
    nlohmann::json j;
    j["name"] = name;
    j["version"] = version;
    j["python_version"] = pythonVersion;
    return j;
}

PackageInfo PackageInfo::fromJson(const nlohmann::json& j) {
    PackageInfo pkg;
    pkg.name = j.value("name", "");
    pkg.version = j.value("version", "");
    pkg.pythonVersion = j.value("python_version", "");
    return pkg;
}

ConfigException::ConfigException(const std::string& message)
    : message_(message) {
}

const char* ConfigException::what() const noexcept {
    return message_.c_str();
}

ConfigManager::ConfigManager(const std::string& configPath)
    : configPath_(configPath)
    , loaded_(false)
    , cacheValid_(false)
    , cachePopulated_(false) {
    
    if (!std::filesystem::exists(configPath_)) {
        LOG_INFO("配置文件不存在，将创建默认配置文件: " + configPath_);
        createDefaultConfig();
    }
}

void ConfigManager::loadConfig() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (loaded_) {
        LOG_INFO("配置已经加载，跳过重复加载");
        return;
    }
    
    std::ifstream file(configPath_);
    
    if (!file.is_open()) {
        LOG_ERROR("配置文件不存在: " + configPath_);
        throw ConfigException("配置文件不存在: " + configPath_);
    }
    
    try {
        file >> config_;
        
        validateConfig();
        
        loaded_ = true;
        cacheValid_ = true;
        cacheTimestamp_ = std::chrono::system_clock::now();
        cachePopulated_ = false;
        
        LOG_INFO("配置文件加载成功: " + configPath_);
    } catch (const nlohmann::json::parse_error& e) {
        LOG_ERROR("JSON 解析错误: " + std::string(e.what()));
        throw ConfigException("JSON 解析错误: " + std::string(e.what()));
    } catch (const std::exception& e) {
        LOG_ERROR("读取配置文件时发生错误: " + std::string(e.what()));
        throw ConfigException("读取配置文件时发生错误: " + std::string(e.what()));
    }
}

void ConfigManager::saveConfig() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    try {
        std::ofstream file(configPath_);
        
        if (!file.is_open()) {
            LOG_ERROR("无法打开配置文件进行写入: " + configPath_);
            throw ConfigException("无法打开配置文件进行写入: " + configPath_);
        }
        
        file << config_.dump(4);
        
        cacheTimestamp_ = std::chrono::system_clock::now();
        cachePopulated_ = false;
        
        LOG_INFO("配置文件保存成功: " + configPath_);
    } catch (const std::exception& e) {
        LOG_ERROR("保存配置文件时发生错误: " + std::string(e.what()));
        throw ConfigException("保存配置文件时发生错误: " + std::string(e.what()));
    }
}

std::string ConfigManager::getCurrentVersion() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!loaded_) {
        throw ConfigException("配置尚未加载");
    }
    
    if (cacheValid_ && cachePopulated_) {
        return cachedCurrentVersion_;
    }
    
    cachedCurrentVersion_ = config_.value("python_version", "");
    return cachedCurrentVersion_;
}

void ConfigManager::setCurrentVersion(const std::string& version) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!loaded_) {
        throw ConfigException("配置尚未加载");
    }

    bool versionExists = config_.contains("python_path") &&
                         config_["python_path"].is_object() &&
                         config_["python_path"].contains(version);

    if (!versionExists) {
        LOG_ERROR("Python 版本不存在: " + version);
        throw ConfigException("Python 版本不存在: " + version);
    }

    config_["python_version"] = version;
    cachedCurrentVersion_ = version;
    LOG_INFO("设置当前 Python 版本: " + version);
}

std::string ConfigManager::getPythonPath(const std::string& version) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!loaded_) {
        throw ConfigException("配置尚未加载");
    }
    
    if (cacheValid_ && cachePopulated_) {
        auto it = cachedPythonPaths_.find(version);
        if (it != cachedPythonPaths_.end()) {
            return it->second;
        }
        LOG_ERROR("未找到 Python 版本路径: " + version);
        throw ConfigException("未找到 Python 版本路径: " + version);
    }
    
    if (!config_.contains("python_path") || !config_["python_path"].is_object()) {
        throw ConfigException("配置中缺少 python_path 字段");
    }
    
    const auto& paths = config_["python_path"];
    if (!paths.contains(version)) {
        LOG_ERROR("未找到 Python 版本路径: " + version);
        throw ConfigException("未找到 Python 版本路径: " + version);
    }
    
    return paths[version].get<std::string>();
}

void ConfigManager::addPythonPath(const std::string& version, const std::string& path) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!loaded_) {
        throw ConfigException("配置尚未加载");
    }
    
    if (!config_.contains("python_path")) {
        config_["python_path"] = nlohmann::json::object();
    }
    
    config_["python_path"][version] = path;
    cachedPythonPaths_[version] = path;
    LOG_INFO("添加 Python 版本路径: " + version + " -> " + path);
}

bool ConfigManager::removePythonPath(const std::string& version) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!loaded_) {
        throw ConfigException("配置尚未加载");
    }
    
    if (!config_.contains("python_path") || !config_["python_path"].contains(version)) {
        LOG_WARNING("尝试删除不存在的 Python 版本: " + version);
        return false;
    }
    
    config_["python_path"].erase(version);
    cachedPythonPaths_.erase(version);
    LOG_INFO("移除 Python 版本路径: " + version);
    return true;
}

std::map<std::string, std::string> ConfigManager::getAllPythonPaths() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!loaded_) {
        throw ConfigException("配置尚未加载");
    }
    
    if (cacheValid_ && cachePopulated_) {
        return std::map<std::string, std::string>(cachedPythonPaths_);
    }
    
    cachedPythonPaths_.clear();
    
    if (config_.contains("python_path") && config_["python_path"].is_object()) {
        for (auto& [version, path] : config_["python_path"].items()) {
            cachedPythonPaths_[version] = path.get<std::string>();
        }
    }
    
    return std::map<std::string, std::string>(cachedPythonPaths_);
}

std::vector<PackageInfo> ConfigManager::getPackages() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!loaded_) {
        throw ConfigException("配置尚未加载");
    }
    
    if (cacheValid_ && cachePopulated_) {
        return std::vector<PackageInfo>(cachedPackages_);
    }
    
    cachedPackages_.clear();
    
    if (config_.contains("packages") && config_["packages"].is_array()) {
        for (const auto& pkgJson : config_["packages"]) {
            cachedPackages_.push_back(PackageInfo::fromJson(pkgJson));
        }
    }
    
    return std::vector<PackageInfo>(cachedPackages_);
}

void ConfigManager::addPackage(const PackageInfo& package) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!loaded_) {
        throw ConfigException("配置尚未加载");
    }
    
    if (!config_.contains("packages")) {
        config_["packages"] = nlohmann::json::array();
    }
    
    auto& packages = config_["packages"];
    for (auto& pkg : packages) {
        if (pkg["name"] == package.name) {
            pkg = package.toJson();
            for (auto& cachedPkg : cachedPackages_) {
                if (cachedPkg.name == package.name) {
                    cachedPkg = package;
                    break;
                }
            }
            LOG_INFO("更新包信息: " + package.name);
            return;
        }
    }
    
    packages.push_back(package.toJson());
    cachedPackages_.push_back(package);
    LOG_INFO("添加包信息: " + package.name);
}

bool ConfigManager::removePackage(const std::string& packageName) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!loaded_) {
        throw ConfigException("配置尚未加载");
    }
    
    if (!config_.contains("packages") || !config_["packages"].is_array()) {
        LOG_WARNING("配置中没有包列表");
        return false;
    }
    
    auto& packages = config_["packages"];
    for (auto it = packages.begin(); it != packages.end(); ++it) {
        if ((*it)["name"] == packageName) {
            packages.erase(it);
            cachedPackages_.erase(
                std::remove_if(cachedPackages_.begin(), cachedPackages_.end(),
                    [&packageName](const PackageInfo& pkg) { return pkg.name == packageName; }),
                cachedPackages_.end()
            );
            LOG_INFO("移除包信息: " + packageName);
            return true;
        }
    }
    
    LOG_WARNING("未找到要移除的包: " + packageName);
    return false;
}

bool ConfigManager::isLoaded() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return loaded_;
}

std::string ConfigManager::getConfigPath() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return configPath_;
}

void ConfigManager::reloadConfig() {
    loaded_ = false;
    config_.clear();
    cachedCurrentVersion_.clear();
    cachedPythonPaths_.clear();
    cachedPackages_.clear();
    cachePopulated_ = false;
    cacheValid_ = false;
    
    loadConfig();
}

void ConfigManager::clearCache() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    cachedCurrentVersion_.clear();
    cachedPythonPaths_.clear();
    cachedPackages_.clear();
    cachePopulated_ = false;
    cacheValid_ = false;
    
    LOG_INFO("缓存已清除");
}

bool ConfigManager::hasPythonVersion(const std::string& version) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!loaded_) {
        return false;
    }
    
    if (cacheValid_ && cachePopulated_) {
        return cachedPythonPaths_.find(version) != cachedPythonPaths_.end();
    }
    
    return config_.contains("python_path") && 
           config_["python_path"].is_object() && 
           config_["python_path"].contains(version);
}

bool ConfigManager::isCacheValid() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return cacheValid_;
}

std::chrono::system_clock::time_point ConfigManager::getCacheTimestamp() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return cacheTimestamp_;
}

void ConfigManager::updateCache() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!loaded_ || !cacheValid_) {
        return;
    }
    
    cachedCurrentVersion_ = config_.value("python_version", "");
    
    cachedPythonPaths_.clear();
    if (config_.contains("python_path") && config_["python_path"].is_object()) {
        for (auto& [version, path] : config_["python_path"].items()) {
            cachedPythonPaths_[version] = path.get<std::string>();
        }
    }
    
    cachedPackages_.clear();
    if (config_.contains("packages") && config_["packages"].is_array()) {
        for (const auto& pkgJson : config_["packages"]) {
            cachedPackages_.push_back(PackageInfo::fromJson(pkgJson));
        }
    }
    
    cachePopulated_ = true;
    LOG_INFO("缓存已更新");
}

bool ConfigManager::needsCacheUpdate() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return !cachePopulated_ || !cacheValid_;
}

void ConfigManager::validateConfig() const {
    if (!config_.contains("python_version")) {
        throw ConfigException("配置缺少必需字段: python_version");
    }
    
    if (!config_.contains("python_path")) {
        throw ConfigException("配置缺少必需字段: python_path");
    }
    
    if (!config_["python_path"].is_object()) {
        throw ConfigException("python_path 必须是对象类型");
    }
    
    if (config_.contains("packages") && !config_["packages"].is_array()) {
        throw ConfigException("packages 必须是数组类型");
    }
}

void ConfigManager::createDefaultConfig() {
    config_ = nlohmann::json{
        {"python_version", ""},
        {"python_path", nlohmann::json::object()},
        {"packages", nlohmann::json::array()}
    };
    
    try {
        std::ofstream file(configPath_);
        
        if (!file.is_open()) {
            LOG_ERROR("无法创建默认配置文件: " + configPath_);
            throw ConfigException("无法创建默认配置文件: " + configPath_);
        }
        
        file << config_.dump(4);
        file.close();
        
        LOG_INFO("默认配置文件创建成功: " + configPath_);
    } catch (const std::exception& e) {
        LOG_ERROR("创建默认配置文件时发生错误: " + std::string(e.what()));
        throw ConfigException("创建默认配置文件时发生错误: " + std::string(e.what()));
    }
    
    loaded_ = true;
    cacheValid_ = true;
    cacheTimestamp_ = std::chrono::system_clock::now();
    cachePopulated_ = false;
}