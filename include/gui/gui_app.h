#ifndef GUI_APP_H
#define GUI_APP_H

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include "../config_manager.h"
#include "../logger.h"

// Forward declarations for DirectX 11
struct ID3D11Device;
struct ID3D11DeviceContext;
struct IDXGISwapChain;
struct ID3D11RenderTargetView;
struct HWND__;
typedef HWND__* HWND;

/**
 * @brief GUI 应用类
 * 
 * 负责管理 ImGui GUI 界面的初始化、渲染和清理
 */
class GuiApp {
public:
    /**
     * @brief 构造函数
     * @param configManager 配置管理器引用
     */
    explicit GuiApp(ConfigManager& configManager);
    
    /**
     * @brief 析构函数
     */
    ~GuiApp();
    
    // 禁止拷贝
    GuiApp(const GuiApp&) = delete;
    GuiApp& operator=(const GuiApp&) = delete;
    
    /**
     * @brief 初始化 GUI 应用
     * @return 是否成功初始化
     */
    bool initialize();
    
    /**
     * @brief 运行 GUI 主循环
     */
    void run();
    
    /**
     * @brief 清理资源
     */
    void cleanup();

private:
    /**
     * @brief 创建 DirectX 11 设备
     * @param hWnd 窗口句柄
     * @return 是否成功创建
     */
    bool createDeviceD3D(HWND hWnd);
    
    /**
     * @brief 清理 DirectX 11 设备
     */
    void cleanupDeviceD3D();
    
    /**
     * @brief 创建渲染目标
     */
    void createRenderTarget();
    
    /**
     * @brief 清理渲染目标
     */
    void cleanupRenderTarget();
    
    /**
     * @brief 渲染 GUI 界面
     */
    void renderGui();
    
    /**
     * @brief 渲染版本管理面板
     */
    void renderVersionPanel();
    
    /**
     * @brief 渲染信息面板
     */
    void renderInfoPanel();
    
    /**
     * @brief 渲染命令面板
     */
    void renderCommandPanel();
    
    /**
     * @brief 渲染日志面板
     */
    void renderLogPanel();
    
    /**
     * @brief 刷新版本列表
     */
    void refreshVersionList();
    
    /**
     * @brief 切换 Python 版本
     * @param version 目标版本
     */
    void switchVersion(const std::string& version);
    
    /**
     * @brief 添加 Python 版本
     */
    void addVersion();
    
    /**
     * @brief 删除 Python 版本
     * @param version 要删除的版本
     */
    void removeVersion(const std::string& version);
    
    /**
     * @brief 执行命令
     * @param command 命令类型 (python/pip)
     * @param args 命令参数
     */
    void executeCommand(const std::string& command, const std::string& args);
    
    /**
     * @brief 添加日志消息
     * @param message 日志消息
     */
    void addLogMessage(const std::string& message);
    
    /**
     * @brief 获取 Python 版本信息
     * @param path Python 安装路径
     * @return 版本号字符串
     */
    std::string getPythonVersion(const std::string& path);
    
    /**
     * @brief 获取 Python 详细版本信息
     * @return 详细版本信息字符串
     */
    std::string getPythonDetailedVersion();
    
    /**
     * @brief 获取已安装的包列表
     * @return 包列表字符串
     */
    std::string getInstalledPackages();
    
    /**
     * @brief 获取可执行文件所在目录
     * @return 可执行文件所在目录的路径
     */
    std::string getExecutableDirectory() const;
    
    // 配置管理器引用
    ConfigManager& configManager_;
    
    // DirectX 11 设备
    ID3D11Device* d3dDevice_;
    ID3D11DeviceContext* d3dDeviceContext_;
    IDXGISwapChain* swapChain_;
    ID3D11RenderTargetView* mainRenderTargetView_;
    
    // 窗口句柄
    HWND hwnd_;
    
    // 窗口尺寸
    unsigned int resizeWidth_;
    unsigned int resizeHeight_;
    bool swapChainOccluded_;
    
    // GUI 状态
    bool initialized_;
    
    // 版本列表
    std::vector<std::string> versionList_;
    int selectedVersionIndex_;
    std::string currentVersion_;
    
    // 命令输入
    std::string commandInput_;
    std::string commandType_;  // "python" or "pip"
    
    // 日志消息
    std::vector<std::string> logMessages_;
    bool autoScrollLog_;
    
    // 搜索包
    std::string packageSearchInput_;
    std::string searchResult_;
    
    // 新版本路径输入
    std::string newVersionPath_;
    
    // 信息缓存
    std::string cachedDetailedVersion_;
    std::string cachedPlatformInfo_;
    std::string cachedPackages_;
    bool infoCacheValid_;
};

#endif // GUI_APP_H