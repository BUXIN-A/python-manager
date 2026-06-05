# 更新日志

## 1.2.1 - 2026-06-05 - 性能优化和BUG修复

### 性能优化
- 优化命令执行完成后的退出速度，从约3秒降低到约100ms
- 优化进程等待超时时间，从 INFINITE 改为 5秒
- 优化I/O线程等待时间，从1秒降低到100ms
- 优化日志输出，只在错误级别时刷新日志文件

### 功能改进
- 更新日志文件存放位置到程序目录的logs文件夹
- 自动创建logs目录（如果不存在）
- 自动创建默认配置文件（如果config.json不存在）
- 修复配置文件路径问题，确保在任何目录都能找到配置文件

### BUG修复
- 修复38个潜在BUG，包括：
  - 内存/资源泄漏（6个）
  - 异常处理（4个）
  - 空指针检查（3个）
  - 边界检查（3个）
  - 字符串处理（2个）
  - 线程安全（1个）
  - 编码问题（2个）
  - 缓存问题（2个）
  - 死锁问题（1个）

### 文件变更
- 修改: src/pythonw_proxy.cpp - 优化进程等待
- 修改: src/command_handler.cpp - 优化进程执行
- 修改: src/proxy_utils.cpp - 优化进程等待和I/O线程
- 修改: src/logger.cpp - 优化日志输出
- 修改: src/config_manager.cpp - 自动创建配置文件
- 修改: src/main.cpp - 配置文件路径修复
- 新增: include/proxy_utils.h - 添加getLogFilePath方法
- 修改: src/proxy_utils.cpp - 实现日志路径生成

## 1.2.0 - 2026-06-05 - 添加 pym python/pythonw/pip 命令功能

### 新增功能
- 添加 `pym python <command>` 命令，执行 Python 命令并显示详细日志
- 添加 `pym pythonw <command>` 命令，执行 Pythonw 命令并显示详细日志
- 添加 `pym pip <command>` 命令，执行 pip 命令并显示详细日志
- 通过 pym 命令调用代理程序时，显示详细的日志信息
- 直接调用代理程序时，仅输出命令结果（不显示日志）

### 技术实现
- 修改 Logger 类，添加 `setConsoleOutput(bool enable)` 方法，用于控制是否输出日志到控制台
- 修改代理程序（python_proxy、pythonw_proxy、pip_proxy），检查 PYM_VERBOSE 环境变量：
  - 如果 PYM_VERBOSE=1，输出详细日志到控制台
  - 否则，只输出到文件，不输出到控制台
- 修改 CommandHandler 类，添加三个新方法：
  - `handlePython()` - 执行 Python 命令
  - `handlePythonw()` - 执行 Pythonw 命令
  - `handlePip()` - 执行 pip 命令
- 添加辅助方法：
  - `getExecutableDirectory()` - 获取可执行文件所在目录
  - `executeProcessWithEnv()` - 执行进程并设置环境变量

### 使用示例
```powershell
# 通过 pym 命令调用（显示详细日志）
pym python --version
pym python -c "print('Hello')"
pym pip list

# 直接调用代理程序（仅显示命令结果）
python --version
python -c "print('Hello')"
pip list
```

### 文件变更
- 修改: `include/logger.h` - 添加 `setConsoleOutput()` 方法声明
- 修改: `src/logger.cpp` - 实现 `setConsoleOutput()` 方法
- 修改: `src/python_proxy.cpp` - 添加 PYM_VERBOSE 环境变量检查
- 修改: `src/pythonw_proxy.cpp` - 添加 PYM_VERBOSE 环境变量检查
- 修改: `src/pip_proxy.cpp` - 添加 PYM_VERBOSE 环境变量检查
- 修改: `include/command_handler.h` - 添加新方法声明
- 修改: `src/command_handler.cpp` - 实现新方法
- 修改: `CMakeLists.txt` - 更新版本号到 1.2.0

## 1.1.0 - 2026-06-05 - 添加可执行文件图标

### 新增功能
- 为 pym.exe 添加专属图标 (pym.ico)
- 为 python.exe 和 pythonw.exe 添加 Python 图标 (python.ico)

### 技术实现
- 创建 resources 目录存放图标文件和资源文件
- 添加资源文件：
  - `resources/pym.rc` - pym.exe 的资源文件
  - `resources/python.rc` - python.exe 的资源文件
  - `resources/pythonw.rc` - pythonw.exe 的资源文件
- 更新 CMakeLists.txt，将资源文件添加到各可执行目标的编译中
- 将 python-manager.exe 输出名称更改为 pym.exe

### 文件变更
- 新增: `resources/pym.ico`
- 新增: `resources/python.ico`
- 新增: `resources/pym.rc`
- 新增: `resources/python.rc`
- 新增: `resources/pythonw.rc`
- 修改: `CMakeLists.txt`