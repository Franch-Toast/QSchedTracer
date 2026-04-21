set(CMAKE_SYSTEM_NAME QNX)
set(CMAKE_SYSTEM_VERSION 7.1.0)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

if(NOT QNX71_TOOLCHAIN_ROOT)
    foreach(_candidate
            "${CMAKE_CURRENT_LIST_DIR}/../../toolchains_qnx"
            "/sandbox/toolchains_qnx")
        if(IS_DIRECTORY "${_candidate}/host/linux/x86_64/usr/bin")
            get_filename_component(QNX71_TOOLCHAIN_ROOT "${_candidate}" ABSOLUTE)
            break()
        endif()
    endforeach()
endif()

if(NOT QNX71_TOOLCHAIN_ROOT)
    message(FATAL_ERROR "QNX 7.1 toolchain not found. Set QNX71_TOOLCHAIN_ROOT.")
endif()

set(QNX_HOST   "${QNX71_TOOLCHAIN_ROOT}/host/linux/x86_64")
set(QNX_TARGET "${QNX71_TOOLCHAIN_ROOT}/target/qnx7")

set(ENV{QNX_HOST}   "${QNX_HOST}")
set(ENV{QNX_TARGET} "${QNX_TARGET}")

set(CMAKE_C_COMPILER   "${QNX_HOST}/usr/bin/aarch64-unknown-nto-qnx7.1.0-gcc-8.3.0")
set(CMAKE_CXX_COMPILER "${QNX_HOST}/usr/bin/aarch64-unknown-nto-qnx7.1.0-g++-8.3.0")
set(CMAKE_AR           "${QNX_HOST}/usr/bin/aarch64-unknown-nto-qnx7.1.0-ar"     CACHE FILEPATH "")
set(CMAKE_RANLIB       "${QNX_HOST}/usr/bin/aarch64-unknown-nto-qnx7.1.0-ranlib" CACHE FILEPATH "")

set(CMAKE_FIND_ROOT_PATH "${QNX_TARGET}")
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

set(QNX_VERSION "71" CACHE STRING "" FORCE)
