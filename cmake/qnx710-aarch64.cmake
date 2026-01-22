# ============================================================================
# QNX 7.1 aarch64 交叉编译工具链
# ============================================================================

set(CMAKE_SYSTEM_NAME QNX)
set(CMAKE_SYSTEM_VERSION 7.1.0)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

# QNX 环境变量
if(NOT DEFINED ENV{QNX_HOST})
    message(FATAL_ERROR "QNX_HOST 未设置。请先执行: source /path/to/qnx710/qnxsdp-env.sh")
endif()

if(NOT DEFINED ENV{QNX_TARGET})
    message(FATAL_ERROR "QNX_TARGET 未设置。请先执行: source /path/to/qnx710/qnxsdp-env.sh")
endif()

set(QNX_HOST $ENV{QNX_HOST})
set(QNX_TARGET $ENV{QNX_TARGET})

# 设置PATH，确保编译器能找到工具
set(ENV{PATH} "${QNX_HOST}/usr/bin:$ENV{PATH}")

# 创建临时的ld符号链接
execute_process(COMMAND ln -sf ${QNX_HOST}/usr/bin/aarch64-unknown-nto-qnx7.1.0-ld /tmp/ld)
set(ENV{PATH} "/tmp:$ENV{PATH}")

# 编译器 (使用完整路径，与build.bash保持一致)
set(CMAKE_C_COMPILER ${QNX_HOST}/usr/bin/aarch64-unknown-nto-qnx7.1.0-gcc-8.3.0)
set(CMAKE_CXX_COMPILER ${QNX_HOST}/usr/bin/aarch64-unknown-nto-qnx7.1.0-g++-8.3.0)

# 链接器
set(CMAKE_LINKER ${QNX_HOST}/usr/bin/aarch64-unknown-nto-qnx7.1.0-ld)

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

