set(CMAKE_SYSTEM_NAME QNX)
set(CMAKE_SYSTEM_VERSION 8.0.0)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

if(NOT QNX80_TOOLCHAIN_ROOT)
    foreach(_candidate
            "${CMAKE_CURRENT_LIST_DIR}/../../toolchains_qnx_sdp8"
            "/sandbox/toolchains_qnx_sdp8")
        if(IS_DIRECTORY "${_candidate}/host/linux/x86_64/usr/bin")
            get_filename_component(QNX80_TOOLCHAIN_ROOT "${_candidate}" ABSOLUTE)
            break()
        endif()
    endforeach()
endif()

if(NOT QNX80_TOOLCHAIN_ROOT)
    message(FATAL_ERROR "QNX 8.0 toolchain not found. Set QNX80_TOOLCHAIN_ROOT.")
endif()

set(QNX_HOST   "${QNX80_TOOLCHAIN_ROOT}/host/linux/x86_64")
set(QNX_TARGET "${QNX80_TOOLCHAIN_ROOT}/target/qnx")

set(ENV{QNX_HOST}   "${QNX_HOST}")
set(ENV{QNX_TARGET} "${QNX_TARGET}")

set(CMAKE_C_COMPILER   "${QNX_HOST}/usr/bin/aarch64-unknown-nto-qnx8.0.0-gcc-12.2.0")
set(CMAKE_CXX_COMPILER "${QNX_HOST}/usr/bin/aarch64-unknown-nto-qnx8.0.0-g++-12.2.0")
set(CMAKE_AR           "${QNX_HOST}/usr/bin/aarch64-unknown-nto-qnx8.0.0-ar"     CACHE FILEPATH "")
set(CMAKE_RANLIB       "${QNX_HOST}/usr/bin/aarch64-unknown-nto-qnx8.0.0-ranlib" CACHE FILEPATH "")

set(CMAKE_FIND_ROOT_PATH "${QNX_TARGET}")
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

set(QNX_VERSION "80" CACHE STRING "" FORCE)
