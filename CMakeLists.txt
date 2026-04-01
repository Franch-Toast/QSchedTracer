cmake_minimum_required(VERSION 3.14)
project(QSchedTracer
    VERSION 3.0.0
    DESCRIPTION "QNX Scheduler Tracer - Ring Mode, QST v4"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 检查 spdlog
if(NOT EXISTS "${CMAKE_SOURCE_DIR}/spdlog/include/spdlog/spdlog.h")
    message(FATAL_ERROR "spdlog not found in ${CMAKE_SOURCE_DIR}/spdlog/")
endif()

# QNX 工具链设置
if(NOT CMAKE_TOOLCHAIN_FILE AND CMAKE_SYSTEM_NAME STREQUAL "QNX")
    set(QNX_HOST "/sandbox/toolchains_qnx/host/linux/x86_64" CACHE PATH "QNX host tools")
    set(QNX_TARGET "/sandbox/toolchains_qnx/target/qnx7" CACHE PATH "QNX target")
    set(CMAKE_C_COMPILER "${QNX_HOST}/usr/bin/aarch64-unknown-nto-qnx7.1.0-gcc-8.3.0")
    set(CMAKE_CXX_COMPILER "${QNX_HOST}/usr/bin/aarch64-unknown-nto-qnx7.1.0-g++-8.3.0")
    set(ENV{QNX_HOST} "${QNX_HOST}")
    set(ENV{QNX_TARGET} "${QNX_TARGET}")
    set(ENV{PATH} "${QNX_HOST}/usr/bin:$ENV{PATH}")
endif()

# 编译选项
if(CMAKE_SYSTEM_NAME STREQUAL "QNX")
    add_definitions(-D_QNX_SOURCE -D__QNXNTO__)
    add_compile_options(-Wall -Wextra -O2 -g -stdlib=libc++)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -stdlib=libc++")
else()
    add_compile_options(-Wall -Wextra -Wpedantic -O2 -g)
endif()

# 平台宏 (默认 QNX 7.1; 构建 8.0 时传 -DLP8797)
option(QNX80 "Build for QNX 8.0 (LP8797)" OFF)
if(QNX80)
    add_definitions(-DLP8797)
else()
    add_definitions(-DLP8650)
endif()

include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/spdlog/include
)

set(TRACER_SOURCES
    src/core/data_buffer.cpp
    src/core/tracer_engine.cpp
    src/data/data_manager.cpp
    src/main.cpp
)

add_executable(qst_tracer ${TRACER_SOURCES})

if(CMAKE_SYSTEM_NAME STREQUAL "QNX")
    target_link_libraries(qst_tracer c++ m)
    target_link_options(qst_tracer PRIVATE -Wl,--no-as-needed)
endif()

install(TARGETS qst_tracer RUNTIME DESTINATION bin)

message(STATUS "")
message(STATUS "  QSchedTracer v${PROJECT_VERSION} (Ring Mode, QST v4)")
message(STATUS "  Platform: ${CMAKE_SYSTEM_NAME} ${CMAKE_SYSTEM_PROCESSOR}")
message(STATUS "  QNX 8.0: ${QNX80}")
message(STATUS "")
