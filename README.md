# Python 版本管理器 (Python Manager)

## 项目简介

Python 版本管理器是一个轻量级的 Python 版本管理工具，帮助用户在不同的 Python 版本之间快速切换，无需手动配置环境变量。它通过代理机制将 `python`、`pythonw` 和 `pip` 命令转发到配置的 Python 版本，实现无缝的版本切换体验。

## 功能特性

- **版本切换**: 快速切换不同的 Python 版本
- **版本列表**: 查看已安装的所有 Python 版本
- **版本信息**: 显示当前 Python 版本的详细信息
- **包查找**: 查找已安装的 Python 包并记录版本信息
- **命令代理**: 自动将 python/pythonw/pip 命令代理到指定版本
- **日志记录**: 完整的日志系统记录操作过程
- **跨平台支持**: 支持 Windows 平台（Linux/macOS 计划支持）

## 项目信息

| 属性 | 值                  |
| -- | ------------------ |
| 名称 | python-manager     |
| 版本 | 1.2.1              |
| 作者 | Pursin (部鑫)        |
| 邮箱 | <buxin0912@qq.com> |
| 语言 | C++ 17             |

## 快速开始

### 安装

1. 从 [Release](release/) 目录下载最新版本的压缩包
2. 解压到任意目录
3. 配置 `config.json` 文件（参考配置说明）
4. 将解压目录添加到系统 PATH 环境变量（可选）

### 基本使用

```bash
# 显示帮助信息
pym --help

# 列出已安装的 Python 版本
pym list

# 切换到指定版本
pym switch 3.10.10

# 查看当前版本信息
pym info

# 查找已安装的包
pym find numpy

# 添加 Python 版本
pym add <path>
```

## 构建说明

### 系统要求

- **操作系统**: Windows 7 及以上
- **编译器**: MSVC 2019/2022 或 GCC/Clang
- **构建工具**: CMake 3.10 及以上

### 构建步骤

#### Windows (Visual Studio)

```powershell
# 1. 创建构建目录
mkdir build
cd build

# 2. 配置 CMake 项目
cmake -G "Visual Studio 17 2022" -A x64 ..

# 3. 构建项目
cmake --build . --config Release

# 4. 输出文件位于 build/bin/Release/
```

#### Windows (MSVC 命令行)

```powershell
# 1. 创建构建目录并配置
mkdir build
cd build
cmake ..

# 2. 构建 Release 版本
cmake --build . --config Release

# 3. 或构建 Debug 版本
cmake --build . --config Debug
```

#### 使用打包脚本

```powershell
# 使用 PowerShell 脚本一键构建和打包
.\build_package.ps1

# 仅构建不打包
.\build_package.ps1 -NoZip

# 清理后重新构建
.\build_package.ps1 -Clean

# 指定构建类型
.\build_package.ps1 -BuildType Debug
```

### 构建目标

| 目标                   | 描述                  |
| -------------------- | ------------------- |
| `python-manager`     | 主管理工具 (pym)         |
| `python_proxy`       | Python 解释器代理        |
| `pythonw_proxy`      | Pythonw 解释器代理（无控制台） |
| `pip_proxy`          | pip 包管理器代理          |
| `logger_lib`         | 日志记录静态库             |
| `config_manager_lib` | 配置管理静态库             |

### 清理命令

```powershell
# 清理构建生成的文件（保留 CMake 配置）
cmake --build build --target clean_all

# 深度清理（删除所有 CMake 生成的文件）
cmake --build build --target clean_deep
```

## 安装说明

### 手动安装

1. 构建项目或下载预编译版本
2. 将以下文件复制到目标目录：
   - `pym.exe` - 主管理程序
   - `python.exe` - Python 代理
   - `pythonw.exe` - Pythonw 代理
   - `pip.exe` - pip 代理
   - `config.json` - 配置文件
3. 编辑 `config.json` 配置 Python 版本路径
4. 将目标目录添加到 PATH 环境变量

### 配置 PATH 环境变量

**方法一：系统环境变量**

1. 打开"系统属性" -> "高级" -> "环境变量"
2. 在"系统变量"中找到 `Path`，点击"编辑"
3. 添加 Python Manager 的安装目录路径

**方法二：用户环境变量**

1. 打开"系统属性" -> "高级" -> "环境变量"
2. 在"用户变量"中新建或编辑 `Path`
3. 添加 Python Manager 的安装目录路径

## 使用说明

### pym 命令

| 命令                     | 说明                   | 示例                                        |
| ---------------------- | -------------------- | ----------------------------------------- |
| `pym --help`           | 显示帮助信息               | `pym --help`                              |
| `pym --version`        | 显示版本号                | `pym --version`                           |
| `pym list`             | 列出已安装的 Python 版本     | `pym list`                                |
| `pym switch <version>` | 切换到指定 Python 版本      | `pym switch 3.10.10`                      |
| `pym add [path]`       | 添加 Python 版本（可选路径参数） | `pym add E:\environment\python\python311` |
| `pym info`             | 查看当前 Python 版本信息     | `pym info`                                |
| `pym gui`              | 启动 GUI 界面（占位功能）      | `pym gui`                                 |
| `pym find <package>`   | 查找已安装的 Python 包      | `pym find numpy`                          |

### 代理命令

| 命令               | 说明                      | 示例                  |
| ---------------- | ----------------------- | ------------------- |
| `python <args>`  | 代理到配置的 Python 版本        | `python script.py`  |
| `pythonw <args>` | 代理到配置的 Pythonw 版本（无控制台） | `pythonw script.py` |
| `pip <args>`     | 代理到配置的 pip 版本           | `pip install numpy` |

### 命令详解

#### pym list

列出配置文件中所有已安装的 Python 版本，并标记当前使用的版本。

```bash
$ pym list
已安装的 Python 版本:

  * 3.13.13 (当前)
    路径: C:\Python313
    3.10.10
    路径: C:\Python310
```

#### pym switch

切换到指定的 Python 版本。版本必须在 `config.json` 的 `python_path` 中配置。

**成功切换示例**:

```bash
$ pym switch 3.10.10
切换 Python 版本到 3.10.10...

成功切换到 Python 3.10.10
路径: C:\Python310
```

**版本不存在示例**:

```bash
$ pym switch 3.11.0
切换 Python 版本到 3.11.0...

错误: Python 版本 3.11.0 未安装
请使用 'pym list' 查看已安装的版本
```

#### pym info

显示当前 Python 版本的详细信息，包括版本号、详细版本信息、平台信息等。

```bash
$ pym info

当前 Python 版本: 3.13.13

Python 可执行文件:
  C:\Python313\python.exe

版本信息:
  Python 3.13.13

详细版本信息:
  3.13.13 (main, ...

平台信息:
  Windows-10-...
```

#### pym find

查找指定包在当前 Python 版本中的安装位置和版本信息，并可选择添加到配置文件。

```bash
$ pym find numpy
搜索包: numpy

找到包:
  名称: numpy
  版本: 2.4.4
  路径: C:\Python313\Lib\site-packages\numpy

是否添加到配置文件? (y/n): y

包信息已添加到配置文件
```

#### pym add

添加新的 Python 版本到配置文件。可以指定路径参数，也可以不指定路径参数打开文件夹选择对话框。

**指定路径参数**:

```bash
$ pym add E:\environment\python\python311

添加 Python 版本...

路径: E:\environment\python\python311
检测到 Python 版本: 3.11.0

成功添加 Python 3.11.0
路径: E:\environment\python\python311

是否切换到此版本? (y/n): y
已切换到 Python 3.11.0
```

**不指定路径参数（打开文件夹选择对话框）**:

```bash
$ pym add
# 将弹出文件夹选择对话框，用户选择 Python 安装目录后自动添加
```

**错误处理示例**:

```bash
$ pym add invalid_path

添加 Python 版本...

路径: invalid_path
错误: 路径不存在
```

```bash
$ pym add C:\invalid_python

添加 Python 版本...

路径: C:\invalid_python
错误: 路径不包含 python.exe，不是有效的 Python 安装目录
```

## 配置文件说明

### config.json 结构

```json
{
    "python_version": "当前使用的 Python 版本",
    "python_path": {
        "版本号": "Python 安装路径",
        ...
    },
    "packages": [
        {
            "name": "包名",
            "python_version": "安装的 Python 版本",
            "version": "包版本"
        },
        ...
    ]
}
```

### 字段说明

| 字段               | 类型     | 必填 | 说明                |
| ---------------- | ------ | -- | ----------------- |
| `python_version` | string | 是  | 当前使用的 Python 版本号  |
| `python_path`    | object | 是  | Python 版本到安装路径的映射 |
| `packages`       | array  | 否  | 已记录的包信息列表         |

### python\_path 字段

每个键值对映射一个 Python 版本到其安装路径：

```json
"python_path": {
    "3.13.13": "C:\\Python313",
    "3.10.10": "C:\\Python310",
    "3.11.0": "C:\\Python311"
}
```

**注意**:

- 路径应指向 Python 安装的根目录（包含 `python.exe` 的目录）
- Windows 路径使用双反斜杠 `\\` 或正斜杠 `/`

### packages 字段

记录已安装包的信息：

```json
"packages": [
    {
        "name": "numpy",
        "python_version": "3.13.13",
        "version": "2.4.4"
    },
    {
        "name": "pandas",
        "python_version": "3.13.13",
        "version": "2.0.0"
    }
]
```

### 配置示例

```json
{
    "python_version": "3.13.13",
    "python_path": {
        "3.13.13": "C:\\Python313",
        "3.10.10": "C:\\Python310",
        "3.11.0": "C:\\Python311"
    },
    "packages": [
        {
            "name": "numpy",
            "python_version": "3.13.13",
            "version": "2.4.4"
        },
        {
            "name": "requests",
            "python_version": "3.13.13",
            "version": "2.31.0"
        }
    ]
}
```

## 常见问题解答

### Q1: 执行 python 命令提示找不到 Python？

**原因**: `config.json` 中的 `python_path` 配置不正确。

**解决方案**:

1. 检查配置的路径是否正确
2. 确保路径指向 Python 安装根目录（包含 `python.exe`）
3. 使用 `pym info` 命令验证配置

### Q2: 如何添加新的 Python 版本？

**解决方案**: 编辑 `config.json` 文件，在 `python_path` 中添加新的版本映射：

```json
"python_path": {
    "3.11.0": "C:\\Python311"
}
```

### Q3: 切换版本后 pip 命令不工作？

**原因**: 目标 Python 版本的 Scripts 目录中没有 `pip.exe`。

**解决方案**:

1. 确保目标 Python 版本已安装 pip
2. pip 通常位于 `<Python安装路径>\Scripts\pip.exe`
3. 可以使用 `python -m pip` 替代 `pip`

### Q4: 配置文件损坏怎么办？

**解决方案**:

1. 删除 `config.json` 文件
2. 程序会自动创建默认配置
3. 参考 `config.example.json` 重新配置

### Q5: 如何查看当前使用的 Python 版本？

**解决方案**: 使用以下任一命令：

```bash
pym info
python --version
```

### Q6: 日志文件在哪里？

**解决方案**: 日志文件位于程序目录的 `logs` 文件夹下（如 `logs\python-manager.log`）。

### Q7: PowerShell 中中文显示乱码怎么办？

**原因**: PowerShell 默认编码可能不是 UTF-8，导致中文输出显示乱码。

**解决方案**:

**方法一：使用 pym.ps1 启动脚本（推荐）**

使用项目提供的 `pym.ps1` 脚本，它会自动设置编码：

```powershell
# 使用 pym.ps1 脚本（自动设置 UTF-8 编码）
.\pym.ps1 list
.\pym.ps1 info
.\pym.ps1 switch 3.10.10
```

**方法二：手动设置编码**

如果直接使用 `pym.exe`，需要在每次会话开始时手动设置编码：

```powershell
# 设置控制台编码为 UTF-8
[Console]::OutputEncoding = [System.Text.Encoding]::UTF8
[Console]::InputEncoding = [System.Text.Encoding]::UTF8

# 然后使用 pym 命令
pym list
pym info
```

**方法三：配置 PowerShell 配置文件**

在 PowerShell 配置文件中添加编码设置，使其永久生效：

```powershell
# 打开 PowerShell 配置文件（如果不存在会提示创建）
notepad $PROFILE

# 在配置文件中添加以下内容：
[Console]::OutputEncoding = [System.Text.Encoding]::UTF8
[Console]::InputEncoding = [System.Text.Encoding]::UTF8
```

**推荐做法**: 将 `pym.ps1` 脚本放在与 `pym.exe` 相同的目录中，并添加到 PATH 环境变量。这样可以直接使用 `pym.ps1` 命令，无需每次手动设置编码。

## 技术栈和依赖

### 技术栈

| 技术            | 版本    | 用途      |
| ------------- | ----- | ------- |
| C++           | C++17 | 主要开发语言  |
| CMake         | 3.10+ | 构建系统    |
| nlohmann/json | 单文件头  | JSON 解析 |

### 依赖库

| 库           | 说明                       |
| ----------- | ------------------------ |
| `json.hpp`  | nlohmann JSON 库，用于配置文件解析 |
| Windows API | 用于 Windows 平台的进程创建       |

### 编译依赖

- **MSVC**: Visual Studio 2019/2022
- **CMake**: 3.10 及以上版本

## 项目结构

```
python-manager/
├── include/              # 头文件目录
│   ├── command_handler.h # 命令处理器头文件
│   ├── config_manager.h  # 配置管理器头文件
│   ├── logger.h          # 日志记录器头文件
│   └── json.hpp          # JSON 解析库
├── src/                  # 源代码目录
│   ├── main.cpp          # 主程序入口
│   ├── command_handler.cpp # 命令处理器实现
│   ├── config_manager.cpp  # 配置管理器实现
│   ├── logger.cpp        # 日志记录器实现
│   ├── python_proxy.cpp  # Python 代理程序
│   ├── pythonw_proxy.cpp # Pythonw 代理程序
│   └── pip_proxy.cpp     # pip 代理程序
├── build/                # 构建输出目录
├── release/              # 发布包目录
├── docs/                 # 文档目录
├── CMakeLists.txt        # CMake 配置文件
├── build_package.ps1     # 打包脚本
├── config.json           # 配置文件
├── config.example.json   # 配置示例
└── README.md             # 项目说明文档
```

## 联系方式

- **作者**: Pursin (部鑫)
- **邮箱**: <buxin0912@qq.com>
- **问题反馈**: 请通过邮件或项目仓库提交 Issue

***

*最后更新: 2026年*
