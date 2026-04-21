cmake_minimum_required(VERSION 3.14)
project(kev_parser
    VERSION 1.0.0
    DESCRIPTION "QNX kev file parser - converts tracelogger output to AI-friendly JSONL/text"
    LANGUAGES C CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_C_STANDARD 11)

# ============================================================================
# QNX version selection
# ============================================================================
set(QNX_VERSION "71" CACHE STRING "QNX version to build against: 71 or 80")
set_property(CACHE QNX_VERSION PROPERTY STRINGS "71" "80")

if(NOT QNX_VERSION STREQUAL "71" AND NOT QNX_VERSION STREQUAL "80")
    message(FATAL_ERROR "QNX_VERSION must be '71' or '80', got '${QNX_VERSION}'")
endif()

# ============================================================================
# Resolve toolchain / bundled dependency paths
# ============================================================================
# Bundled deps path (shipped with this project for standalone builds)
set(KEV_BUNDLED_DEPS "${CMAKE_SOURCE_DIR}/deps")

# Search order: explicit cache var → sibling of project → /sandbox → bundled deps
if(NOT QNX71_TOOLCHAIN_ROOT)
    foreach(_candidate
            "${CMAKE_SOURCE_DIR}/../toolchains_qnx"
            "/sandbox/toolchains_qnx")
        if(EXISTS "${_candidate}/target/qnx7/usr/include/sys/traceparser.h")
            get_filename_component(QNX71_TOOLCHAIN_ROOT "${_candidate}" ABSOLUTE)
            break()
        endif()
    endforeach()
endif()

if(NOT QNX80_TOOLCHAIN_ROOT)
    foreach(_candidate
            "${CMAKE_SOURCE_DIR}/../toolchains_qnx_sdp8"
            "/sandbox/toolchains_qnx_sdp8")
        if(EXISTS "${_candidate}/target/qnx/usr/include/sys/traceparser.h")
            get_filename_component(QNX80_TOOLCHAIN_ROOT "${_candidate}" ABSOLUTE)
            break()
        endif()
    endforeach()
endif()

# Pick active toolchain based on QNX_VERSION
if(QNX_VERSION STREQUAL "71")
    set(QNX_TOOLCHAIN_ROOT "${QNX71_TOOLCHAIN_ROOT}")
    if(QNX_TOOLCHAIN_ROOT)
        set(QNX_TARGET_DIR "${QNX_TOOLCHAIN_ROOT}/target/qnx7")
    endif()
else()
    set(QNX_TOOLCHAIN_ROOT "${QNX80_TOOLCHAIN_ROOT}")
    if(QNX_TOOLCHAIN_ROOT)
        set(QNX_TARGET_DIR "${QNX_TOOLCHAIN_ROOT}/target/qnx")
    endif()
endif()

# For QNX cross-compilation, the full toolchain is always required
if(CMAKE_SYSTEM_NAME STREQUAL "QNX")
    if(NOT QNX_TOOLCHAIN_ROOT OR NOT EXISTS "${QNX_TARGET_DIR}")
        message(FATAL_ERROR
            "QNX ${QNX_VERSION} toolchain not found for cross-compilation.\n"
            "Set QNX${QNX_VERSION}_TOOLCHAIN_ROOT or place toolchains_qnx* next to this project.")
    endif()
endif()

# ============================================================================
# Build mode detection
# ============================================================================
if(CMAKE_SYSTEM_NAME STREQUAL "QNX")
    set(KEV_BUILD_MODE "qnx_cross")
else()
    set(KEV_BUILD_MODE "linux_host")
endif()

message(STATUS "")
message(STATUS "=== kev_parser build configuration ===")
message(STATUS "  QNX_VERSION:      ${QNX_VERSION}")
message(STATUS "  Build mode:       ${KEV_BUILD_MODE}")
message(STATUS "  Toolchain root:   ${QNX_TOOLCHAIN_ROOT}")
message(STATUS "  QNX target dir:   ${QNX_TARGET_DIR}")

# ============================================================================
# Find libtraceparser.a
# ============================================================================
if(KEV_BUILD_MODE STREQUAL "linux_host")
    set(_lib_arch "x86_64")
else()
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|arm64")
        set(_lib_arch "aarch64le")
    else()
        set(_lib_arch "x86_64")
    endif()
endif()

# Search: toolchain first, then bundled deps
set(TRACEPARSER_LIB "")
if(QNX_TARGET_DIR AND EXISTS "${QNX_TARGET_DIR}/${_lib_arch}/usr/lib/libtraceparser.a")
    set(TRACEPARSER_LIB "${QNX_TARGET_DIR}/${_lib_arch}/usr/lib/libtraceparser.a")
    message(STATUS "  libtraceparser:   ${TRACEPARSER_LIB} (from toolchain)")
elseif(KEV_BUILD_MODE STREQUAL "linux_host")
    set(_bundled_ver "qnx${QNX_VERSION}")
    set(_bundled_lib "${KEV_BUNDLED_DEPS}/${_bundled_ver}/lib/libtraceparser.a")
    if(EXISTS "${_bundled_lib}")
        set(TRACEPARSER_LIB "${_bundled_lib}")
        message(STATUS "  libtraceparser:   ${TRACEPARSER_LIB} (bundled)")
    endif()
endif()

if(NOT TRACEPARSER_LIB OR NOT EXISTS "${TRACEPARSER_LIB}")
    message(FATAL_ERROR
        "libtraceparser.a not found.\n"
        "Searched: toolchain (${QNX_TARGET_DIR}/${_lib_arch}/usr/lib/) and "
        "bundled deps (${KEV_BUNDLED_DEPS}/qnx${QNX_VERSION}/lib/).\n"
        "Either install a QNX toolchain or ensure deps/ directory is intact.")
endif()
message(STATUS "")

# ============================================================================
# QNX compatibility layer (Linux host only)
# ============================================================================
if(KEV_BUILD_MODE STREQUAL "linux_host")
    add_library(qnx_shim STATIC src/qnx_shim.c)
    target_include_directories(qnx_shim PRIVATE
        ${CMAKE_SOURCE_DIR}/include
    )
    target_compile_options(qnx_shim PRIVATE -fno-stack-protector)
endif()

# ============================================================================
# kev_parser static library (core parsing logic)
# ============================================================================
set(KEV_PARSER_SOURCES
    src/kev_parser.cpp
    src/raw_kev_ts.cpp
    src/name_tracker.cpp
    src/event_names.cpp
    src/kercall_fields.cpp
    src/output_writer.cpp
    src/stats.cpp
    src/filter.cpp
)

add_library(kev_parser_core STATIC ${KEV_PARSER_SOURCES})

if(KEV_BUILD_MODE STREQUAL "linux_host")
    target_include_directories(kev_parser_core PUBLIC
        ${CMAKE_SOURCE_DIR}/include
    )
    target_compile_definitions(kev_parser_core PUBLIC KEV_LINUX_HOST=1)
else()
    target_include_directories(kev_parser_core PUBLIC
        ${CMAKE_SOURCE_DIR}/include
        ${QNX_TARGET_DIR}/usr/include
    )
endif()

target_compile_definitions(kev_parser_core PUBLIC KEV_QNX_VERSION=${QNX_VERSION})

# ============================================================================
# Main executable
# ============================================================================
add_executable(kev_parser src/main.cpp)
target_link_libraries(kev_parser PRIVATE kev_parser_core)

if(KEV_BUILD_MODE STREQUAL "linux_host")
    target_link_libraries(kev_parser PRIVATE
        qnx_shim
        ${TRACEPARSER_LIB}
    )
    target_link_options(kev_parser PRIVATE -fno-stack-protector)
else()
    target_link_libraries(kev_parser PRIVATE ${TRACEPARSER_LIB})
    if(QNX_VERSION STREQUAL "71")
        target_link_options(kev_parser PRIVATE -stdlib=libc++)
        target_link_libraries(kev_parser PRIVATE c++ m)
    endif()
endif()

# ============================================================================
# Install
# ============================================================================
install(TARGETS kev_parser RUNTIME DESTINATION bin)

# ============================================================================
# Build summary
# ============================================================================
message(STATUS "Targets:")
message(STATUS "  kev_parser   - main kev-to-JSON converter")
message(STATUS "")
