# ============================================================================
# QSchedTracer - QNX 调度追踪器
# 
# CMake 构建配置
# 
# 使用方法:
#   # QNX 交叉编译 (aarch64)
#   mkdir build && cd build
#   cmake -DCMAKE_TOOLCHAIN_FILE=../cmake/qnx710-aarch64.cmake ..
#   make
#   
#   # QNX 交叉编译 (x86_64)
#   cmake -DCMAKE_TOOLCHAIN_FILE=../cmake/qnx710-x86_64.cmake ..
#   make
# ============================================================================

cmake_minimum_required(VERSION 3.14)
project(QSchedTracer
    VERSION 2.0.0
    DESCRIPTION "QNX Scheduler Tracer - Flight Recorder Mode"
    LANGUAGES CXX
)

# ============================================================================
# 依赖检查
# ============================================================================

# 检查 spdlog 库
if(NOT EXISTS "${CMAKE_SOURCE_DIR}/spdlog/include/spdlog/spdlog.h")
    message(FATAL_ERROR "spdlog library not found in ${CMAKE_SOURCE_DIR}/spdlog/
    Please ensure spdlog is placed in the project's spdlog/ directory")
endif()

# 检查 nlohmann/json 库
if(NOT EXISTS "${CMAKE_SOURCE_DIR}/third_party/nlohmann/json.hpp")
    message(FATAL_ERROR "nlohmann/json library not found in ${CMAKE_SOURCE_DIR}/third_party/nlohmann/
    Please ensure nlohmann/json.hpp is placed in the project's third_party/nlohmann/ directory")
endif()

# ============================================================================
# C++ 标准
# ============================================================================
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# QNX 工具链设置 (如果没有通过工具链文件设置)
if(NOT CMAKE_TOOLCHAIN_FILE AND CMAKE_SYSTEM_NAME STREQUAL "QNX")
    # 设置 QNX 工具链路径
    set(QNX_HOST "/sandbox/toolchains_qnx/host/linux/x86_64" CACHE PATH "QNX host tools path")
    set(QNX_TARGET "/sandbox/toolchains_qnx/target/qnx7" CACHE PATH "QNX target path")

    # 设置编译器 (需要根据目标架构调整)
    set(CMAKE_C_COMPILER "${QNX_HOST}/usr/bin/aarch64-unknown-nto-qnx7.1.0-gcc-8.3.0")
    set(CMAKE_CXX_COMPILER "${QNX_HOST}/usr/bin/aarch64-unknown-nto-qnx7.1.0-g++-8.3.0")

    # 设置环境变量
    set(ENV{QNX_HOST} "${QNX_HOST}")
    set(ENV{QNX_TARGET} "${QNX_TARGET}")
    set(ENV{PATH} "${QNX_HOST}/usr/bin:$ENV{PATH}")
endif()

# ============================================================================
# 编译选项
# ============================================================================

# QNX 特定定义和编译选项
if(CMAKE_SYSTEM_NAME STREQUAL "QNX")
    add_definitions(
        -D_QNX_SOURCE
        -D__QNXNTO__
    )

    # QNX 特定的编译选项
    add_compile_options(
        -Wall
        -Wextra
        -O2
        -g
        -stdlib=libc++
    )

    # 设置 C++ 标准库
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -stdlib=libc++")
else()
    # 非 QNX 系统的编译选项
    add_compile_options(
        -Wall
        -Wextra
        -Wpedantic
        -O2
        -g
    )
endif()

# ============================================================================
# 包含目录
# ============================================================================
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/spdlog/include
    ${CMAKE_SOURCE_DIR}/third_party
)

# ============================================================================
# 源文件
# ============================================================================
set(TRACER_SOURCES
    # 核心模块
    src/core/data_buffer.cpp
    src/core/tracer_engine.cpp
    
    # 配置模块
    src/config/config_loader.cpp
    
    # 触发器模块
    src/trigger/kernel_event_trigger.cpp
    src/trigger/trigger_manager.cpp
    
    # 事件模块
    src/event/event_manager.cpp
    
    # 数据模块
    src/data/data_manager.cpp
    
    # 主入口
    src/main.cpp
)

# ============================================================================
# 可执行文件
# ============================================================================
add_executable(qst_tracer ${TRACER_SOURCES})

# QNX 特定的链接选项
if(CMAKE_SYSTEM_NAME STREQUAL "QNX")
    target_link_libraries(qst_tracer
        c++
        m
    )

    # 设置链接标志
    target_link_options(qst_tracer PRIVATE
        -Wl,--no-as-needed
    )
endif()

# ============================================================================
# 安装
# ============================================================================
install(TARGETS qst_tracer
    RUNTIME DESTINATION bin
)

install(FILES tools/qst_parse.py
    DESTINATION tools
    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
                GROUP_READ GROUP_EXECUTE
                WORLD_READ WORLD_EXECUTE
)

install(FILES config/default.json
    DESTINATION etc/qst
    RENAME qst_config.json
)

# ============================================================================
# 打印配置信息
# ============================================================================
message(STATUS "")
message(STATUS "╔══════════════════════════════════════════════════════════╗")
message(STATUS "║           QSchedTracer 配置信息                          ║")
message(STATUS "╠══════════════════════════════════════════════════════════╣")
message(STATUS "║ 版本:        ${PROJECT_VERSION}")
message(STATUS "║ C++ 标准:    C++${CMAKE_CXX_STANDARD}")
message(STATUS "║ 系统:        ${CMAKE_SYSTEM_NAME}")
message(STATUS "║ 处理器:      ${CMAKE_SYSTEM_PROCESSOR}")
message(STATUS "║ 编译器:      ${CMAKE_CXX_COMPILER}")
message(STATUS "╚══════════════════════════════════════════════════════════╝")
message(STATUS "")
