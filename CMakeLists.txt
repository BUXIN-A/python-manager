# =============================================================================
# Python 版本管理器 (pym) - CMake 构建配置文件
# =============================================================================
# 项目：Python 版本管理器
# 版本：1.3.0
# 描述：提供 Python 版本管理和代理功能，使用 Dear ImGui 的 GUI 版本
# =============================================================================

cmake_minimum_required(VERSION 3.16)

# 项目信息
project(PythonManager VERSION 1.3.3 LANGUAGES CXX)

# =============================================================================
# 1. 全局配置
# =============================================================================

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 设置默认构建类型为 Release（如果未指定）
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "构建类型" FORCE)
    message(STATUS "未指定构建类型，默认使用: ${CMAKE_BUILD_TYPE}")
endif()

# 输出当前构建类型
message(STATUS "构建类型: ${CMAKE_BUILD_TYPE}")

# =============================================================================
# 2. 编译选项配置
# =============================================================================

# Windows 特定编译定义
if(WIN32)
    # 定义 Windows 版本，支持 Windows 7 及以上
    add_compile_definitions(_WIN32_WINNT=0x0601)
    # 减少 Windows.h 包含的头文件，加快编译速度
    add_compile_definitions(WIN32_LEAN_AND_MEAN)
    # 禁用 Windows.h 中的 min/max 宏，避免与标准库冲突
    add_compile_definitions(NOMINMAX)
    # 使用 Unicode 字符集
    add_compile_definitions(UNICODE _UNICODE)
endif()

# MSVC 编译器特定选项
if(MSVC)
    # 设置运行时库为多线程（根据构建类型自动选择静态或动态链接）
    # 静态链接运行时库，避免依赖 MSVC 运行时
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

    # 编译选项：
    # /W4 - 警告级别 4（较高）
    # /utf-8 - 源文件和执行字符集使用 UTF-8
    # /WX - 将警告视为错误（可选，根据需要启用）
    # /MP - 多进程编译（加快编译速度）
    add_compile_options(/W4 /utf-8 /MP)

    # Debug 特定选项
    # /Od - 禁用优化
    # /Zi - 生成调试信息
    # /RTC1 - 运行时错误检查
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /Od /Zi /RTC1")

    # Release 特定选项
    # /O2 - 最大化速度优化
    # /DNDEBUG - 禁用断言
    # 注意：禁用 /GL 以避免与栈保护相关的优化问题
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2 /DNDEBUG")

    # 链接器 Release 选项
    # /OPT:REF - 移除未引用的函数
    # /OPT:ICF - 合并相同的函数
    # 注意：禁用 /LTCG 以避免与栈保护相关的优化问题
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /OPT:REF /OPT:ICF")
    set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} /OPT:REF /OPT:ICF")
else()
    # GCC/Clang 编译选项
    add_compile_options(-Wall -Wextra -Wpedantic)

    # Debug 特定选项
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -g3")

    # Release 特定选项
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -DNDEBUG")
endif()

# =============================================================================
# 3. 输出目录配置
# =============================================================================

# 设置输出目录（按构建类型分目录）
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/${CMAKE_BUILD_TYPE})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/${CMAKE_BUILD_TYPE})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/${CMAKE_BUILD_TYPE})

# 为所有配置设置输出目录（用于多配置生成器，如 Visual Studio）
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin/Debug)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin/Release)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/lib/Debug)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/lib/Release)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/lib/Debug)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/lib/Release)

# =============================================================================
# 4. 头文件目录
# =============================================================================

# 包含项目头文件目录
include_directories(${CMAKE_SOURCE_DIR}/include)

# =============================================================================
# 5. ImGui 库配置
# =============================================================================

# ImGui 源文件
set(IMGUI_SOURCES
    external/imgui/imgui.cpp
    external/imgui/imgui_draw.cpp
    external/imgui/imgui_tables.cpp
    external/imgui/imgui_widgets.cpp
    external/imgui/backends/imgui_impl_win32.cpp
    external/imgui/backends/imgui_impl_dx11.cpp
)

# ImGui 头文件目录
include_directories(${CMAKE_SOURCE_DIR}/external/imgui)
include_directories(${CMAKE_SOURCE_DIR}/external/imgui/backends)

# =============================================================================
# 6. 库目标构建
# =============================================================================

# -----------------------------------------------------------------------------
# 6.1 logger 库 - 日志记录库
# -----------------------------------------------------------------------------
add_library(logger_lib STATIC
    src/logger.cpp
)

# 设置库的输出名称
set_target_properties(logger_lib PROPERTIES
    OUTPUT_NAME "logger"
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
)

# logger 库编译选项
if(MSVC)
    target_compile_options(logger_lib PRIVATE /W4 /utf-8)
else()
    target_compile_options(logger_lib PRIVATE -Wall -Wextra -Wpedantic)
endif()

# logger 库编译定义
target_compile_definitions(logger_lib PRIVATE
    $<$<CONFIG:Debug>:DEBUG>
    $<$<CONFIG:Release>:NDEBUG>
)

# -----------------------------------------------------------------------------
# 6.2 config_manager 库 - 配置管理库
# -----------------------------------------------------------------------------
add_library(config_manager_lib STATIC
    src/config_manager.cpp
)

# 设置库的输出名称
set_target_properties(config_manager_lib PROPERTIES
    OUTPUT_NAME "config_manager"
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
)

# config_manager 依赖 logger
target_link_libraries(config_manager_lib PUBLIC logger_lib)

# config_manager 库编译选项
if(MSVC)
    target_compile_options(config_manager_lib PRIVATE /W4 /utf-8)
else()
    target_compile_options(config_manager_lib PRIVATE -Wall -Wextra -Wpedantic)
endif()

# config_manager 库编译定义
target_compile_definitions(config_manager_lib PRIVATE
    $<$<CONFIG:Debug>:DEBUG>
    $<$<CONFIG:Release>:NDEBUG>
)

# -----------------------------------------------------------------------------
# 6.3 proxy_utils 库 - 代理程序工具库
# -----------------------------------------------------------------------------
add_library(proxy_utils_lib STATIC
    src/proxy_utils.cpp
)

# 设置库的输出名称
set_target_properties(proxy_utils_lib PROPERTIES
    OUTPUT_NAME "proxy_utils"
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
)

# proxy_utils 依赖 config_manager 和 logger
target_link_libraries(proxy_utils_lib PUBLIC
    config_manager_lib
    logger_lib
)

# proxy_utils 库编译选项
if(MSVC)
    target_compile_options(proxy_utils_lib PRIVATE /W4 /utf-8)
else()
    target_compile_options(proxy_utils_lib PRIVATE -Wall -Wextra -Wpedantic)
endif()

# proxy_utils 库编译定义
target_compile_definitions(proxy_utils_lib PRIVATE
    $<$<CONFIG:Debug>:DEBUG>
    $<$<CONFIG:Release>:NDEBUG>
)

# =============================================================================
# 7. GUI 目标构建 (Dear ImGui + DirectX 11)
# =============================================================================

# -----------------------------------------------------------------------------
# 7.1 pym-gui - GUI 版本管理工具 (Dear ImGui)
# -----------------------------------------------------------------------------
add_executable(pym-gui
    src/gui/gui_main.cpp
    ${IMGUI_SOURCES}
    resources/gui.rc
)

# 设置输出名称
set_target_properties(pym-gui PROPERTIES
    OUTPUT_NAME "pym-gui"
    VERSION ${PROJECT_VERSION}
    WIN32_EXECUTABLE TRUE
)

# 包含目录
target_include_directories(pym-gui PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/gui
    ${CMAKE_SOURCE_DIR}/external/imgui
    ${CMAKE_SOURCE_DIR}/external/imgui/backends
)

# 链接库
target_link_libraries(pym-gui PRIVATE
    config_manager_lib
    logger_lib
    d3d11.lib
    dxgi.lib
    shell32.lib  # 用于文件夹选择对话框
    ole32.lib    # 用于 COM 接口
)

# pym-gui 编译选项
if(MSVC)
    target_compile_options(pym-gui PRIVATE /W4 /utf-8)
else()
    target_compile_options(pym-gui PRIVATE -Wall -Wextra -Wpedantic)
endif()

# pym-gui 编译定义
target_compile_definitions(pym-gui PRIVATE
    $<$<CONFIG:Debug>:DEBUG>
    $<$<CONFIG:Release>:NDEBUG>
)

message(STATUS "GUI 目标 pym-gui 已配置 (Dear ImGui + DirectX 11)")

# =============================================================================
# 8. 可执行文件目标构建 (命令行版本)
# =============================================================================

# -----------------------------------------------------------------------------
# 8.1 python-manager (pym) - 主管理工具
# -----------------------------------------------------------------------------
add_executable(python-manager
    src/command_handler.cpp
    src/main.cpp
    resources/pym.rc
)

# 设置输出名称为 pym
set_target_properties(python-manager PROPERTIES
    OUTPUT_NAME "pym"
    VERSION ${PROJECT_VERSION}
)

# 链接库
target_link_libraries(python-manager PRIVATE
    config_manager_lib
    logger_lib
)

# python-manager 编译选项
if(MSVC)
    target_compile_options(python-manager PRIVATE /W4 /utf-8)
else()
    target_compile_options(python-manager PRIVATE -Wall -Wextra -Wpedantic)
endif()

# python-manager 编译定义
target_compile_definitions(python-manager PRIVATE
    $<$<CONFIG:Debug>:DEBUG>
    $<$<CONFIG:Release>:NDEBUG>
)

# -----------------------------------------------------------------------------
# 8.2 python_proxy - Python 解释器代理
# -----------------------------------------------------------------------------
add_executable(python_proxy
    src/python_proxy.cpp
    resources/python.rc
)

# 设置输出名称为 python
set_target_properties(python_proxy PROPERTIES
    OUTPUT_NAME "python"
    VERSION ${PROJECT_VERSION}
)

# 链接库
target_link_libraries(python_proxy PRIVATE
    proxy_utils_lib
    config_manager_lib
    logger_lib
)

# python_proxy 编译选项
if(MSVC)
    target_compile_options(python_proxy PRIVATE /W4 /utf-8)
else()
    target_compile_options(python_proxy PRIVATE -Wall -Wextra -Wpedantic)
endif()

# python_proxy 编译定义
target_compile_definitions(python_proxy PRIVATE
    $<$<CONFIG:Debug>:DEBUG>
    $<$<CONFIG:Release>:NDEBUG>
)

# -----------------------------------------------------------------------------
# 8.3 pythonw_proxy - Pythonw 解释器代理（无控制台窗口）
# -----------------------------------------------------------------------------
add_executable(pythonw_proxy
    src/pythonw_proxy.cpp
    resources/pythonw.rc
)

# 设置输出名称为 pythonw
set_target_properties(pythonw_proxy PROPERTIES
    OUTPUT_NAME "pythonw"
    VERSION ${PROJECT_VERSION}
)

# Windows 特定设置：无控制台窗口
if(WIN32)
    set_target_properties(pythonw_proxy PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
endif()

# 链接库
target_link_libraries(pythonw_proxy PRIVATE
    proxy_utils_lib
    config_manager_lib
    logger_lib
)

# pythonw_proxy 编译选项
if(MSVC)
    target_compile_options(pythonw_proxy PRIVATE /W4 /utf-8)
else()
    target_compile_options(pythonw_proxy PRIVATE -Wall -Wextra -Wpedantic)
endif()

# pythonw_proxy 编译定义
target_compile_definitions(pythonw_proxy PRIVATE
    $<$<CONFIG:Debug>:DEBUG>
    $<$<CONFIG:Release>:NDEBUG>
)

# -----------------------------------------------------------------------------
# 8.4 pip_proxy - pip 包管理器代理
# -----------------------------------------------------------------------------
add_executable(pip_proxy
    src/pip_proxy.cpp
)

# 设置输出名称为 pip
set_target_properties(pip_proxy PROPERTIES
    OUTPUT_NAME "pip"
    VERSION ${PROJECT_VERSION}
)

# 链接库
target_link_libraries(pip_proxy PRIVATE
    proxy_utils_lib
    config_manager_lib
    logger_lib
)

# pip_proxy 编译选项
if(MSVC)
    target_compile_options(pip_proxy PRIVATE /W4 /utf-8)
else()
    target_compile_options(pip_proxy PRIVATE -Wall -Wextra -Wpedantic)
endif()

# pip_proxy 编译定义
target_compile_definitions(pip_proxy PRIVATE
    $<$<CONFIG:Debug>:DEBUG>
    $<$<CONFIG:Release>:NDEBUG>
)

# =============================================================================
# 9. 安装规则
# =============================================================================

# 安装可执行文件
install(TARGETS python-manager python_proxy pythonw_proxy pip_proxy pym-gui
    RUNTIME DESTINATION bin
    COMPONENT Runtime
)

# 安装库文件
install(TARGETS logger_lib config_manager_lib proxy_utils_lib
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    COMPONENT Development
)

# 安装头文件
install(FILES
    include/logger.h
    include/config_manager.h
    include/command_handler.h
    include/proxy_utils.h
    include/json.hpp
    DESTINATION include
    COMPONENT Development
)

# 安装配置文件模板
install(FILES
    config.json
    DESTINATION bin
    COMPONENT Runtime
    RENAME config.json.template
)

# 安装 README 文件
install(FILES
    README.md
    DESTINATION .
    COMPONENT Documentation
)

# =============================================================================
# 10. 清理规则
# =============================================================================

# 添加清理目标（用于清理构建生成的文件）
add_custom_target(clean_all
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}/bin
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}/lib
    COMMAND ${CMAKE_COMMAND} -E remove -f ${CMAKE_BINARY_DIR}/*.log
    COMMENT "清理所有构建生成的文件"
)

# 添加深度清理目标（清理所有 CMake 生成的文件）
add_custom_target(clean_deep
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target clean
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}/CMakeFiles
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}/bin
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}/lib
    COMMAND ${CMAKE_COMMAND} -E remove -f ${CMAKE_BINARY_DIR}/CMakeCache.txt
    COMMAND ${CMAKE_COMMAND} -E remove -f ${CMAKE_BINARY_DIR}/cmake_install.cmake
    COMMAND ${CMAKE_COMMAND} -E remove -f ${CMAKE_BINARY_DIR}/Makefile
    COMMAND ${CMAKE_COMMAND} -E remove -f ${CMAKE_BINARY_DIR}/*.vcxproj
    COMMAND ${CMAKE_COMMAND} -E remove -f ${CMAKE_BINARY_DIR}/*.sln
    COMMAND ${CMAKE_COMMAND} -E remove -f ${CMAKE_BINARY_DIR}/*.log
    COMMENT "深度清理所有 CMake 生成的文件"
)

# =============================================================================
# 11. 打包配置
# =============================================================================

# 设置 CPack 打包信息
set(CPACK_PACKAGE_NAME "PythonManager")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Python 版本管理器")
set(CPACK_PACKAGE_VENDOR "PythonManager Team")
set(CPACK_PACKAGE_CONTACT "support@pythonmanager.com")

# Windows 特定打包设置
if(WIN32)
    set(CPACK_GENERATOR "ZIP;NSIS")
    set(CPACK_NSIS_DISPLAY_NAME "Python Manager")
    set(CPACK_NSIS_PACKAGE_NAME "Python Manager")
    set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
else()
    set(CPACK_GENERATOR "TGZ;DEB;RPM")
endif()

# 包含 CPack 模块
include(CPack)

# =============================================================================
# 12. 输出配置信息
# =============================================================================

message(STATUS "")
message(STATUS "========================================")
message(STATUS "项目配置信息")
message(STATUS "========================================")
message(STATUS "项目名称: ${PROJECT_NAME}")
message(STATUS "项目版本: ${PROJECT_VERSION}")
message(STATUS "C++ 标准: ${CMAKE_CXX_STANDARD}")
message(STATUS "构建类型: ${CMAKE_BUILD_TYPE}")
message(STATUS "源代码目录: ${CMAKE_SOURCE_DIR}")
message(STATUS "构建目录: ${CMAKE_BINARY_DIR}")
message(STATUS "可执行文件输出目录: ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
message(STATUS "库文件输出目录: ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}")
message(STATUS "")
message(STATUS "目标:")
message(STATUS "  - python-manager (pym): Python 版本管理工具")
message(STATUS "  - python: Python 解释器代理")
message(STATUS "  - pythonw: Pythonw 解释器代理")
message(STATUS "  - pip: pip 包管理器代理")
message(STATUS "  - pym-gui: GUI 版本管理工具 (Dear ImGui + DirectX 11)")
message(STATUS "  - logger_lib: 日志记录库")
message(STATUS "  - config_manager_lib: 配置管理库")
message(STATUS "")
message(STATUS "构建命令:")
message(STATUS "  Release: cmake --build ${CMAKE_BINARY_DIR} --config Release")
message(STATUS "  Debug:   cmake --build ${CMAKE_BINARY_DIR} --config Debug")
message(STATUS "")
message(STATUS "清理命令:")
message(STATUS "  清理构建文件: cmake --build ${CMAKE_BINARY_DIR} --target clean_all")
message(STATUS "  深度清理:     cmake --build ${CMAKE_BINARY_DIR} --target clean_deep")
message(STATUS "========================================")
message(STATUS "")