# ============================================================================
# QNX 8.0 aarch64 交叉编译工具链
# ============================================================================

set(CMAKE_SYSTEM_NAME QNX)
set(CMAKE_SYSTEM_VERSION 8.0.0)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

# QNX 环境变量
if(NOT DEFINED ENV{QNX_HOST})
    message(FATAL_ERROR "QNX_HOST 未设置。请先执行: source /path/to/qnx800/qnxsdp-env.sh")
endif()

if(NOT DEFINED ENV{QNX_TARGET})
    message(FATAL_ERROR "QNX_TARGET 未设置。请先执行: source /path/to/qnx800/qnxsdp-env.sh")
endif()

set(QNX_HOST $ENV{QNX_HOST})
set(QNX_TARGET $ENV{QNX_TARGET})

set(ENV{PATH} "${QNX_HOST}/usr/bin:$ENV{PATH}")

# 编译器
set(CMAKE_C_COMPILER ${QNX_HOST}/usr/bin/aarch64-unknown-nto-qnx8.0.0-gcc)
set(CMAKE_CXX_COMPILER ${QNX_HOST}/usr/bin/aarch64-unknown-nto-qnx8.0.0-g++)

# 链接器
set(CMAKE_LINKER ${QNX_HOST}/usr/bin/aarch64-unknown-nto-qnx8.0.0-ld)

# GCC collect2 needs plain 'ld' in a -B directory.
set(_QNX_TOOL_DIR "${CMAKE_BINARY_DIR}/_qnx_tools")
file(MAKE_DIRECTORY "${_QNX_TOOL_DIR}")
execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink
    "${QNX_HOST}/usr/bin/aarch64-unknown-nto-qnx8.0.0-ld"
    "${_QNX_TOOL_DIR}/ld")
set(CMAKE_C_FLAGS_INIT   "-B${_QNX_TOOL_DIR}/")
set(CMAKE_CXX_FLAGS_INIT "-B${_QNX_TOOL_DIR}/")

# 查找路径
set(CMAKE_FIND_ROOT_PATH ${QNX_TARGET}/aarch64le)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# 系统包含路径
include_directories(SYSTEM
    ${QNX_TARGET}/usr/include
    ${QNX_TARGET}/aarch64le/usr/include
)
