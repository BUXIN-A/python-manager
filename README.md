# Python Manager (pym)

<p align="center">
  <strong>轻量级 Python 版本管理工具</strong><br>
  Windows · C++17 · Dear ImGui · 零依赖
</p>

---

## 概述

Python Manager（简称 **pym**）是一款面向 Windows 平台的 Python 版本管理工具。通过代理机制将 `python`、`pythonw` 和 `pip` 命令透明转发到用户指定的 Python 版本，实现**零配置切换**——无需修改系统环境变量，无需管理员权限。

同时提供基于 [Dear ImGui](https://github.com/ocornut/imgui) 的图形界面 `pym-gui`，覆盖全部命令行功能。

## 功能

| 功能 | CLI | GUI |
|------|:---:|:---:|
| 版本列表 / 切换 / 添加 / 删除 | `pym list` / `switch` / `add` | 版本管理面板 |
| 当前版本信息 | `pym info` | 版本信息面板 |
| 已安装包搜索 | `pym find <pkg>` | 命令执行面板 |
| 执行 Python / pip 命令 | `pym python` / `pip` | 命令执行面板 |
| 操作日志 | 文件日志 (`logs/`) | 日志输出面板 |

## 快速开始

### 获取

```bash
# 克隆源码
git clone https://github.com/BUXIN-A/python-manager.git
cd python-manager

# 或直接下载 Release 包
```

### 构建

```bash
# 配置 + 编译（Release）
cmake -B build && cmake --build build --config Release

# 输出文件位于 build/bin/Release/
#   pym.exe        — 命令行管理工具
#   pym-gui.exe    — 图形界面
#   python.exe     — Python 代理
#   pythonw.exe    — Pythonw 代理（无控制台）
#   pip.exe        — pip 代理
```

**构建要求：**

- Windows 7+ / MSVC 2019+ / CMake 3.16+
- 无需第三方库（ImGui 内置于 `external/`）

### 安装部署

将以下文件复制到目标目录，并将该目录加入系统 `PATH`：

```
pym.exe          # 主程序
pym-gui.exe      # 图形界面
python.exe       # Python 代理
pythonw.exe      # Pythonw 代理
pip.exe          # pip 代理
config.json      # 配置文件（首次运行自动创建）
```

## 使用方法

### 命令行

```
pym <command> [arguments]

命令:
  list                列出已安装的 Python 版本
  switch <version>    切换到指定版本
  add [path]          添加新版本（支持文件夹选择对话框）
  info                显示当前版本详细信息
  find <package>      搜索已安装的包
  python <args>       执行 Python 命令（带详细日志）
  pythonw <args>      执行 Pythonw 命令（带详细日志）
  pip <args>          执行 pip 命令（带详细日志）
  --version           显示版本号
  --help              显示帮助信息
```

**使用示例：**

```bash
# 列出所有版本
$ pym list
  * 3.13.13 (当前)    E:\environment\python\python313
    3.10.10          E:\environment\python\python310

# 切换版本
$ pym switch 3.10.10
成功切换到 Python 3.10.10

# 添加版本（GUI 选择路径或指定路径）
$ pym add E:\environment\python\python311
成功添加 Python 3.11.0

# 通过 pym 执行命令（显示详细日志）
$ pym python --version
$ pym pip install numpy

# 直接使用代理（仅输出结果）
$ python script.py
$ pip list
```

### 图形界面

```bash
pym-gui
# 或双击 pym-gui.exe
```

GUI 界面包含 5 个面板，首次启动自动排列布局：

```
┌──────────────┬──────────────────────────┐
│              │                          │
│  版本管理     │     版本信息               │
│              │                          │
├──────────────┼─────────────┬────────────┤
│              │             │            │
│  命令执行     │   日志输出    │    帮助     │
│              │             │            │
└──────────────┴─────────────┴────────────┘
```

## 配置说明

配置文件 `config.json` 位于程序所在目录，首次运行时自动创建：

```jsonc
{
    "python_version": "3.13.13",          // 当前使用的版本
    "python_path": {                        // 版本 → 安装路径映射
        "3.13.13": "E:\\python\\python313",
        "3.10.10": "E:\\python\\python310"
    },
    "packages": []                           // 记录的包信息（可选）
}
```

| 字段 | 类型 | 必填 | 说明 |
|---|---|:---:|---|
| `python_version` | string | 是 | 当前激活的版本号 |
| `python_path` | object | 是 | 版本号到安装根目录的映射 |
| `packages` | array | 否 | 通过 `pym find` 记录的包信息 |

## 项目结构

```
python-manager/
├── external/imgui/         # ImGui 库（内嵌）
├── include/                # 头文件
│   ├── command_handler.h
│   ├── config_manager.h
│   ├── logger.h
│   └── proxy_utils.h
├── src/                    # 源码
│   ├── main.cpp            # pym 入口
│   ├── command_handler.cpp
│   ├── config_manager.cpp
│   ├── logger.cpp
│   ├── python_proxy.cpp    # python.exe
│   ├── pythonw_proxy.cpp   # pythonw.exe
│   ├── pip_proxy.cpp       # pip.exe
│   └── gui/gui_main.cpp    # pym-gui 入口
├── resources/              # 图标与资源
├── CMakeLists.txt
├── config.json             # 运行时配置
└── README.md
```

## 技术细节

| 项目 | 说明 |
|---|---|
| 语言标准 | C++17 |
| 构建系统 | CMake 3.16+ |
| JSON 解析 | nlohmann/json（单头文件） |
| GUI 框架 | Dear ImGui + DirectX 11 |
| 代理机制 | CreatePipe + CreateProcess（Windows API） |
| 日志系统 | 自研 Logger（文件 + 控制台，多级别） |
| 字符编码 | UTF-8（内部）/ UTF-16（Windows API 边界） |

## 常见问题

**Q: 执行 python 命令提示找不到 Python？**
A: 检查 `config.json` 中 `python_path` 的路径是否正确指向包含 `python.exe` 的目录。使用 `pym info` 验证。

**Q: PowerShell 中文乱码？**
A: 在 PowerShell 中设置 `[Console]::OutputEncoding = [System.Text.Encoding]::UTF8`，或将程序目录加入 PATH 后使用。

**Q: 日志文件在哪里？**
A: `logs/` 目录下（相对于程序所在目录），包括 `python-manager.log`、`python-proxy.log` 等。

**Q: 如何添加新版本？**
A: 使用 `pym add <path>` 命令，或在 GUI 中点击"添加"按钮选择目录。程序会自动检测版本号。

## 许可证

MIT License &copy; 2026 Pusin (部鑫)

---

<p align="center">
  <a href="mailto:buxin0912@qq.com">buxin0912@qq.com</a>
</p>
