// Python Manager GUI - Dear ImGui 实现
// 使用 Win32 API + DirectX 11 作为渲染后端

#include "config_manager.h"
#include "logger.h"

#include <windows.h>
#include <d3d11.h>
#include <shlobj.h>

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"

#include <string>
#include <vector>
#include <filesystem>
#include <winuser.h>

// UTF-8 转 Wide String（用于 Windows API 显示中文）
std::wstring utf8ToWide(const std::string& str) {
    if (str.empty()) return L"";
    int len = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, nullptr, 0);
    std::wstring wstr(len - 1, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, &wstr[0], len);
    return wstr;
}

// 全局变量
static ID3D11Device*            g_pd3dDevice = nullptr;
static ID3D11DeviceContext*     g_pd3dDeviceContext = nullptr;
static IDXGISwapChain*          g_pSwapChain = nullptr;
static bool                     g_SwapChainOccluded = false;
static UINT                     g_ResizeWidth = 0, g_ResizeHeight = 0;
static ID3D11RenderTargetView*  g_mainRenderTargetView = nullptr;

bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

std::string getExecutableDirectory() {
    char path[MAX_PATH];
    GetModuleFileNameA(NULL, path, MAX_PATH);
    std::string::size_type pos = std::string(path).find_last_of("\\/");
    return std::string(path).substr(0, pos);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    try {
        // 初始化日志系统
        std::string exeDir = getExecutableDirectory();
        std::filesystem::create_directories(exeDir + "\\logs");
        std::string logPath = exeDir + "\\logs\\pym-gui.log";
        Logger& logger = Logger::getInstance();
        logger.setFileOutput(true, logPath);
        logger.setConsoleOutput(false);

        ImGui_ImplWin32_EnableDpiAwareness();

        std::string configPath = exeDir + "\\config.json";
        if (!std::filesystem::exists(configPath)) {
            std::ofstream ofs(configPath);
            ofs << "{\"python_version\":\"\",\"python_path\":{},\"packages\":[]}";
            ofs.close();
        }
        ConfigManager configManager(configPath);
        configManager.loadConfig();

        WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"Python Manager GUI", nullptr };
        ::RegisterClassExW(&wc);

        HWND hwnd = ::CreateWindowW(wc.lpszClassName, L"Python Manager - pym-gui", WS_OVERLAPPEDWINDOW,
                                     100, 100, 1280, 800, nullptr, nullptr, wc.hInstance, nullptr);
        if (!hwnd) return 1;

        if (!CreateDeviceD3D(hwnd)) { CleanupDeviceD3D(); ::UnregisterClassW(wc.lpszClassName, wc.hInstance); return 1; }

        ::ShowWindow(hwnd, SW_SHOWDEFAULT);
        ::UpdateWindow(hwnd);

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO(); (void)io;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

        ImGui::StyleColorsDark();

        ImFontConfig fontCfg = {};
        fontCfg.SizePixels = 16.0f;
        const char* fontPaths[] = {
            "C:\\Windows\\Fonts\\msyh.ttc",
            "C:\\Windows\\Fonts\\simhei.ttf",
            "C:\\Windows\\Fonts\\simsun.ttc"
        };
        bool fontLoaded = false;
        for (const auto& fp : fontPaths) {
            if (std::filesystem::exists(fp)) {
                io.Fonts->AddFontFromFileTTF(fp, 16.0f, &fontCfg, io.Fonts->GetGlyphRangesChineseSimplifiedCommon());
                fontLoaded = true; break;
            }
        }
        if (!fontLoaded) io.Fonts->AddFontDefault(&fontCfg);

        ImGui_ImplWin32_Init(hwnd);
        ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

        // GUI 状态
        std::vector<std::string> versionList;
        int selectedVersionIndex = -1;
        std::string currentVersion = configManager.getCurrentVersion();
        static char commandInputBuf[1024] = "";
        static char packageSearchBuf[256] = "";
        static char newVersionPathBuf[1024] = "";
        std::string commandType = "python";
        std::vector<std::string> logMessages;
        std::string searchResult;

        auto refreshVersionList = [&]() {
            versionList.clear();
            auto paths = configManager.getAllPythonPaths();
            for (const auto& pair : paths) versionList.push_back(pair.first);
            currentVersion = configManager.getCurrentVersion();
            selectedVersionIndex = -1;
            for (int i = 0; i < static_cast<int>(versionList.size()); i++) {
                if (versionList[i] == currentVersion) { selectedVersionIndex = i; break; }
            }
        };

        refreshVersionList();

        auto addLogMessage = [&](const std::string& msg) {
            logMessages.push_back(msg);
            if (logMessages.size() > 500) logMessages.erase(logMessages.begin());
            logger.info(msg);
        };

        auto syncExecuteCommand = [&](const std::string& cmd) -> std::string {
            SECURITY_ATTRIBUTES sa = { sizeof(sa), NULL, TRUE };
            HANDLE hReadPipe, hWritePipe;
            if (!CreatePipe(&hReadPipe, &hWritePipe, &sa, 0)) return "";
            SetHandleInformation(hReadPipe, HANDLE_FLAG_INHERIT, 0);
            STARTUPINFOA si = {}; si.cb = sizeof(si); si.dwFlags = STARTF_USESTDHANDLES;
            si.hStdOutput = si.hStdError = hWritePipe; si.hStdInput = NULL;
            PROCESS_INFORMATION pi = {};
            std::string fullCmd = cmd;
            if (!CreateProcessA(NULL, const_cast<char*>(fullCmd.c_str()), NULL, NULL, TRUE,
                                CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
                CloseHandle(hReadPipe); CloseHandle(hWritePipe); return "";
            }
            CloseHandle(hWritePipe);
            char buffer[4096]; std::string output; DWORD bytesRead;
            while (true) {
                DWORD avail = 0;
                if (!PeekNamedPipe(hReadPipe, NULL, 0, NULL, &avail, NULL) || avail == 0) {
                    if (WaitForSingleObject(pi.hProcess, 50) != WAIT_TIMEOUT) break;
                    continue;
                }
                if (ReadFile(hReadPipe, buffer, sizeof(buffer) - 1, &bytesRead, NULL) && bytesRead > 0) {
                    buffer[bytesRead] = '\0'; output += buffer;
                } else { break; }
            }
            CloseHandle(hReadPipe);
            WaitForSingleObject(pi.hProcess, 3000);
            CloseHandle(pi.hProcess); CloseHandle(pi.hThread);
            return output;
        };

        addLogMessage("Python Manager GUI 已启动");
        addLogMessage("当前版本: " + currentVersion);

        bool firstFrame = true;

        // 主循环
        bool done = false;
        while (!done) {
            MSG msg;
            while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
                ::TranslateMessage(&msg); ::DispatchMessage(&msg);
                if (msg.message == WM_QUIT) done = true;
            }
            if (done) break;

            if (g_SwapChainOccluded && g_pSwapChain->Present(0, DXGI_PRESENT_TEST) == DXGI_STATUS_OCCLUDED) {
                ::Sleep(10); continue;
            }
            g_SwapChainOccluded = false;

            if (g_ResizeWidth != 0 && g_ResizeHeight != 0) {
                CleanupRenderTarget();
                g_pSwapChain->ResizeBuffers(0, g_ResizeWidth, g_ResizeHeight, DXGI_FORMAT_UNKNOWN, 0);
                g_ResizeWidth = g_ResizeHeight = 0;
                CreateRenderTarget();
            }

            // 轮询异步任务结果（预留）

            ImGui_ImplDX11_NewFrame();
            ImGui_ImplWin32_NewFrame();
            ImGui::NewFrame();

            // Dockspace
            {
                ImGuiWindowFlags flags = ImGuiWindowFlags_NoDocking;
                const ImGuiViewport* viewport = ImGui::GetMainViewport();
                ImGui::SetNextWindowPos(viewport->WorkPos);
                ImGui::SetNextWindowSize(viewport->WorkSize);
                ImGui::SetNextWindowViewport(viewport->ID);
                ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
                ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
                flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
                flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
                ImGui::Begin("##DockSpace", nullptr, flags);
                ImGui::PopStyleVar(2);
                ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
                ImGui::DockSpace(dockspace_id, ImVec2(0, 0), ImGuiDockNodeFlags_PassthruCentralNode);
                ImGui::End();
            }

            // 首帧设置各面板初始位置（5个面板，两行布局）
            if (firstFrame) {
                firstFrame = false;
                ImVec2 vp = ImGui::GetMainViewport()->Size;
                // 第一行：版本管理(左) | 版本信息(右)
                ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Once);
                ImGui::SetNextWindowSize(ImVec2(vp.x * 0.38f, vp.y * 0.48f), ImGuiCond_Once);
                ImGui::SetNextWindowPos(ImVec2(vp.x * 0.39f, 0), ImGuiCond_Once);
                ImGui::SetNextWindowSize(ImVec2(vp.x * 0.61f, vp.y * 0.48f), ImGuiCond_Once);
                // 第二行：命令执行 | 日志输出 | 帮助
                ImGui::SetNextWindowPos(ImVec2(0, vp.y * 0.49f), ImGuiCond_Once);
                ImGui::SetNextWindowSize(ImVec2(vp.x * 0.36f, vp.y * 0.51f), ImGuiCond_Once);
                ImGui::SetNextWindowPos(ImVec2(vp.x * 0.37f, vp.y * 0.49f), ImGuiCond_Once);
                ImGui::SetNextWindowSize(ImVec2(vp.x * 0.34f, vp.y * 0.51f), ImGuiCond_Once);
                ImGui::SetNextWindowPos(ImVec2(vp.x * 0.72f, vp.y * 0.49f), ImGuiCond_Once);
                ImGui::SetNextWindowSize(ImVec2(vp.x * 0.28f, vp.y * 0.51f), ImGuiCond_Once);
            }

            // ===== 第一行 =====

            // 版本管理面板
            if (ImGui::Begin("版本管理")) {
                ImGui::Text("已安装的 Python 版本:");
                ImGui::Separator();
                if (ImGui::BeginListBox("##version_list", ImVec2(-FLT_MIN, 5 * ImGui::GetTextLineHeightWithSpacing()))) {
                    for (int i = 0; i < static_cast<int>(versionList.size()); i++) {
                        bool isSelected = (i == selectedVersionIndex);
                        std::string label = versionList[i];
                        if (versionList[i] == currentVersion) label += " (当前)";
                        if (ImGui::Selectable(label.c_str(), isSelected)) selectedVersionIndex = i;
                        ImGui::SameLine();
                        ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "- %s", configManager.getPythonPath(versionList[i]).c_str());
                    }
                    ImGui::EndListBox();
                }
                ImGui::Separator();
                if (ImGui::Button("切换版本", ImVec2(120, 0))) {
                    if (selectedVersionIndex >= 0 && selectedVersionIndex < static_cast<int>(versionList.size())) {
                        std::string target = versionList[selectedVersionIndex];
                        if (target != currentVersion) {
                            configManager.setCurrentVersion(target);
                            currentVersion = target; refreshVersionList();
                            addLogMessage("已切换到 Python " + target);
                        } else { addLogMessage("已经是当前版本"); }
                    } else { addLogMessage("请先选择一个版本"); }
                }
                ImGui::SameLine();
                if (ImGui::Button("刷新列表", ImVec2(120, 0))) { refreshVersionList(); addLogMessage("列表已刷新"); }
                ImGui::Separator();
                ImGui::Text("添加新版本:");
                ImGui::InputText("##new_path", newVersionPathBuf, sizeof(newVersionPathBuf));
                ImGui::SameLine();
                if (ImGui::Button("浏览...")) {
                    BROWSEINFOA bi = {}; bi.lpszTitle = "选择 Python 安装目录";
                    LPITEMIDLIST pidl = SHBrowseForFolderA(&bi);
                    if (pidl) { char p[MAX_PATH]; SHGetPathFromIDListA(pidl, p); strcpy_s(newVersionPathBuf, sizeof(newVersionPathBuf), p); CoTaskMemFree(pidl); }
                }
                if (ImGui::Button("添加", ImVec2(120, 0))) {
                    std::string np(newVersionPathBuf);
                    if (!np.empty() && std::filesystem::exists(np) && std::filesystem::exists(np + "\\python.exe")) {
                        std::string vo = syncExecuteCommand("\"" + np + "\\python.exe\" --version");
                        std::string v;
                        if (vo.find("Python ") != std::string::npos) { v = vo.substr(7); while (!v.empty() && (v.back()=='\r'||v.back()=='\n')) v.pop_back(); }
                        if (!v.empty()) { configManager.addPythonPath(v, np); refreshVersionList(); addLogMessage("已添加 Python " + v); memset(newVersionPathBuf, 0, sizeof(newVersionPathBuf)); }
                        else addLogMessage("无法获取版本号");
                    } else if (!np.empty()) addLogMessage(np.empty() ? "请输入路径" : (std::filesystem::exists(np) ? "路径不含 python.exe" : "路径不存在"));
                }
                ImGui::SameLine();
                if (ImGui::Button("删除选中", ImVec2(120, 0))) {
                    if (selectedVersionIndex >= 0 && selectedVersionIndex < static_cast<int>(versionList.size())) {
                        std::string target = versionList[selectedVersionIndex];
                        if (target != currentVersion) { configManager.removePythonPath(target); refreshVersionList(); addLogMessage("已删除 " + target); }
                        else addLogMessage("不能删除当前使用的版本");
                    } else addLogMessage("请先选择一个版本");
                }
                ImGui::End();
            }

            // 版本信息面板
            if (ImGui::Begin("版本信息")) {
                ImGui::Text("当前版本: %s", currentVersion.c_str());
                ImGui::Text("安装路径: %s", configManager.getPythonPath(currentVersion).empty() ? "未配置" : configManager.getPythonPath(currentVersion).c_str());
                ImGui::Separator();
                std::string pp = configManager.getPythonPath(currentVersion);
                if (!pp.empty()) {
                    ImGui::Text("可执行文件: %s\\python.exe", pp.c_str());
                    ImGui::Text("pip 路径: %s\\Scripts\\pip.exe", pp.c_str());
                }
                ImGui::End();
            }

            // ===== 第二行 =====

            // 命令执行面板
            if (ImGui::Begin("命令执行")) {
                if (ImGui::RadioButton("Python", commandType == "python")) commandType = "python";
                ImGui::SameLine();
                if (ImGui::RadioButton("pip", commandType == "pip")) commandType = "pip";
                ImGui::Separator();
                ImGui::Text("命令参数:");
                ImGui::InputText("##command_input", commandInputBuf, sizeof(commandInputBuf));
                ImGui::SameLine();
                if (ImGui::Button("执行", ImVec2(80, 0))) {
                    std::string ci(commandInputBuf);
                    if (!ci.empty()) {
                        std::string pp = configManager.getPythonPath(currentVersion);
                        if (!pp.empty()) {
                            std::string ep = (commandType == "python") ? pp + "\\python.exe" : pp + "\\Scripts\\pip.exe";
                            std::string fc = "\"" + ep + "\" " + ci;
                            addLogMessage("执行: " + fc);
                            std::string out = syncExecuteCommand(fc);
                            if (!out.empty()) addLogMessage("输出: " + out);
                            memset(commandInputBuf, 0, sizeof(commandInputBuf));
                        } else addLogMessage("当前版本路径未配置");
                    } else addLogMessage("请输入命令参数");
                }
                ImGui::Separator();
                ImGui::Text("搜索包:");
                ImGui::InputText("##package_search", packageSearchBuf, sizeof(packageSearchBuf));
                ImGui::SameLine();
                if (ImGui::Button("搜索", ImVec2(80, 0))) {
                    std::string ps(packageSearchBuf);
                    if (!ps.empty()) {
                        std::string pp = configManager.getPythonPath(currentVersion);
                        if (!pp.empty()) {
                            searchResult = syncExecuteCommand("\"" + pp + "\\Scripts\\pip.exe\" show " + ps);
                            if (searchResult.empty()) searchResult = "未找到包: " + ps;
                            addLogMessage("搜索包: " + ps);
                        } else searchResult = "当前版本路径未配置";
                    }
                }
                if (!searchResult.empty()) { ImGui::Separator(); ImGui::TextWrapped("%s", searchResult.c_str()); }
                ImGui::End();
            }

            // 日志面板
            if (ImGui::Begin("日志输出")) {
                if (ImGui::Button("清除日志", ImVec2(100, 0))) logMessages.clear();
                ImGui::SameLine();
                static bool autoScroll = true;
                ImGui::Checkbox("自动滚动", &autoScroll);
                ImGui::Separator();
                ImGui::BeginChild("LogScroll", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
                for (const auto& m : logMessages) ImGui::TextWrapped("%s", m.c_str());
                if (autoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) ImGui::SetScrollHereY(1.0f);
                ImGui::EndChild();
                ImGui::End();
            }

            // 帮助面板
            if (ImGui::Begin("帮助")) {
                ImGui::Text("Python Manager v1.3.3");
                ImGui::Separator();
                ImGui::Text("快捷操作:");
                ImGui::BulletText("切换版本: 选择后点击\"切换版本\"");
                ImGui::BulletText("添加版本: 输入路径或点击\"浏览...\"");
                ImGui::BulletText("执行命令: 选择 Python/pip 后输入参数");
                ImGui::BulletText("搜索包: 输入包名点击\"搜索\"");
                ImGui::Separator();
                ImGui::Text("配置文件: %s", configPath.c_str());
                ImGui::Text("日志文件: %s", logPath.c_str());
                ImGui::Separator();
                ImGui::TextColored(ImVec4(0.5f, 0.8f, 1.0f, 1.0f), "作者: Pusin (部鑫)");
                ImGui::TextColored(ImVec4(0.5f, 0.8f, 1.0f, 1.0f), "邮箱: buxin0912@qq.com");
                ImGui::End();
            }

            // 渲染
            ImGui::Render();
            float clear_color[4] = { 0.06f, 0.06f, 0.06f, 1.0f };
            g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
            g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color);
            ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

            g_pSwapChain->Present(1, 0);
        }

        ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
        CleanupDeviceD3D();
        ::DestroyWindow(hwnd);
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
        logger.info("Python Manager GUI 正常退出");

    } catch (const std::exception& e) {
        MessageBoxW(NULL, utf8ToWide(e.what()).c_str(), L"Python Manager GUI Error", MB_ICONERROR);
        return 1;
    } catch (...) {
        MessageBoxW(NULL, L"Unknown error", L"Python Manager GUI Error", MB_ICONERROR);
        return 1;
    }
    return 0;
}

bool CreateDeviceD3D(HWND hWnd) {
    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0; sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60; sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1; sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL levels[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };
    HRESULT res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, levels, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res == DXGI_ERROR_UNSUPPORTED)
        res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, 0, levels, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res != S_OK) return false;

    IDXGIFactory* factory = nullptr;
    if (SUCCEEDED(g_pSwapChain->GetParent(IID_PPV_ARGS(&factory)))) {
        factory->MakeWindowAssociation(hWnd, DXGI_MWA_NO_ALT_ENTER);
        factory->Release();
    }
    CreateRenderTarget();
    return true;
}

void CleanupDeviceD3D() {
    CleanupRenderTarget();
    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = nullptr; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
}

void CreateRenderTarget() {
    ID3D11Texture2D* bb = nullptr;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&bb));
    g_pd3dDevice->CreateRenderTargetView(bb, nullptr, &g_mainRenderTargetView);
    bb->Release();
}

void CleanupRenderTarget() {
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = nullptr; }
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam)) return true;
    switch (msg) {
    case WM_SIZE:
        if (wParam == SIZE_MINIMIZED) return 0;
        g_ResizeWidth = (UINT)LOWORD(lParam); g_ResizeHeight = (UINT)HIWORD(lParam);
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) return 0;
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0); return 0;
    }
    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}
