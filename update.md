# 更新日志

## 1.3.3 - 2026-06-06 - GUI 精简与文档重构

### 变更
- **实现 `pym gui` 命令** — 使用 ShellExecuteEx 启动 pym-gui.exe，支持文件存在性检查与错误提示
- **移除"刷新详细信息"按钮** — 版本信息面板改为即时显示（版本号、路径、可执行文件位置），切换版本时自动刷新
- **移除 AsyncCommand 异步执行器** — 删除不再需要的异步命令基础设施，精简 GUI 代码
- **重写 README.md** — 采用更专业的项目文档格式
- **全量重新编译** — 清理 build 目录后从零构建所有目标

### 文件变更
- 修改: `src/command_handler.cpp` — 实现 `pym gui` 命令（启动 pym-gui.exe）
- 修改: `src/gui/gui_main.cpp` — 精简版本信息面板，移除异步相关代码
- 修改: `CMakeLists.txt` — 版本号更新到 1.3.3
- 修改: `include/command_handler.h` — 版本号更新到 1.3.3
- 重写: `README.md` — 专业级项目文档
- 更新: `update.md` — 添加本条目

## 1.3.2 - 2026-06-06 - GUI 稳定性修复和功能增强

### BUG修复
- **修复 GUI 白屏闪退** — 添加 try-catch 异常捕获，修复 ConfigManager 未加载错误
- **修复 MessageBox 中文乱码** — 改用 MessageBoxW + utf8ToWide() UTF-16 转换
- **修复 ImGui 中文显示为 ???** — 加载 Windows 系统中文字体（微软雅黑/黑体/宋体）
- **修复"刷新详细信息"导致窗口卡死** — 使用 std::async 异步执行命令

### 新增功能
- 添加帮助面板（版本号、快捷操作、配置路径、作者信息）
- 首次启动自动排列面板位置（5个面板两行布局）

### 布局
- 第一行：版本管理 | 版本信息
- 第二行：命令执行 | 日志输出 | 帮助

## 1.3.1 - 2026-06-06 - GUI 初步修复

### 新增功能
- 基于 Dear ImGui 的 GUI 界面（替代 Qt 方案）
- Win32 API + DirectX 11 渲染后端
- 5个功能面板：版本管理、版本信息、命令执行、包搜索、日志输出

## 1.3.0 - 2026-06-06 - GUI 框架引入

- 从 Qt 迁移至 Dear ImGui，零外部依赖

## 1.2.1 - 2026-06-05 - 性能优化和BUG修复

### 性能优化
- 退出速度从 ~3s 降低到 ~100ms
- 进程等待超时优化（INFINITE → 5s）
- I/O 线程等待优化（1s → 100ms）

### 功能改进
- 日志文件存放于程序目录 `logs/` 文件夹
- 自动创建 logs 目录和默认配置文件
- 配置文件路径问题修复（任意目录运行均可定位）

### BUG修复
- 修复 38 个潜在 BUG（内存泄漏/异常处理/空指针/边界检查/字符串处理/线程安全/编码/缓存/死锁）

## 1.2.0 - 2026-06-05 - pym python/pythonw/pip 命令

- `pym python <command>` / `pym pythonw <command>` / `pym pip <command>`
- 通过 pym 调用时显示详细日志；直接调用代理时仅输出结果

## 1.1.0 - 2026-06-05 - 可执行文件图标

- pym.exe → pym.ico
- python.exe / pythonw.exe → python.ico

## 1.0.0 - 初始版本

- 核心命令：list / switch / info / find / add
- 代理机制：python / pythonw / pip
- JSON 配置管理
