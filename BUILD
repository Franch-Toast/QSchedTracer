load("@deeproute_build_tools//rules/modules:platform_cc_wrapper.bzl", "platform_cc_library", "platform_cc_binary")
load("@deeproute_build_tools//rules/pkg:deeproute_release.bzl", "deeproute_release_package")
load("@deeproute_build_tools//toolchains:defs.bzl", "deeproute_register_toolchains")

package(default_visibility = ["//visibility:public"])

deeproute_register_toolchains()

# 平台宏定义：LP8797 = QNX 8.0, LP8650 = QNX 7.1
# 匹配优先级: vehicle-target (精确) > chip-type (宽泛) > default
_PLATFORM_DEFINES = select({
    "@deeproute_build_tools//:LP8797-V1-SHARE_setting": ["LP8797"],
    "@deeproute_build_tools//:LP8650-V1-SHARE_setting": ["LP8650"],
    "@deeproute_build_tools//:LP8650-OS-SHARE_setting": ["LP8650"],
    "@deeproute_build_tools//:sa8797_setting": ["LP8797"],
    "//conditions:default": ["LP8650"],
})

# ============================================================================
# QSchedTracer 日志适配层
# ============================================================================

platform_cc_library(
    name = "qst_log",
    hdrs = ["include/qst/log.hpp"],
    includes = ["include"],
    defines = ["BAZEL_BUILD"],
    deps = ["@spdlog//:spdlog"],
    visibility = ["//visibility:private"],
)

# ============================================================================
# QSchedTracer 配置类型
# ============================================================================

platform_cc_library(
    name = "qst_config",
    hdrs = ["include/qst/config/types.hpp"],
    copts = ["-std=c++17", "-O2", "-Wall", "-Wextra"],
    includes = ["include"],
    defines = ["BAZEL_BUILD"] + _PLATFORM_DEFINES,
)

# ============================================================================
# QSchedTracer 数据缓冲区 (元数据容器)
# ============================================================================

platform_cc_library(
    name = "qst_data_buffer",
    hdrs = [
        "include/qst/core/data_buffer.hpp",
        "include/qst/types.hpp",
    ],
    copts = ["-std=c++17", "-O2", "-Wall", "-Wextra"],
    includes = ["include"],
    defines = ["BAZEL_BUILD"] + _PLATFORM_DEFINES,
    deps = [":qst_log"],
)

# ============================================================================
# QSchedTracer 数据管理器 (QST v4 写入)
# ============================================================================

platform_cc_library(
    name = "qst_data",
    srcs = ["src/data/data_manager.cpp"],
    hdrs = ["include/qst/data/data_manager.hpp"],
    copts = ["-std=c++17", "-O2", "-Wall", "-Wextra"],
    includes = ["include"],
    defines = ["BAZEL_BUILD"] + _PLATFORM_DEFINES,
    deps = [
        ":qst_data_buffer",
        ":qst_log",
    ],
)

# ============================================================================
# QSchedTracer 核心引擎 (Ring Mode + mmap)
# ============================================================================

platform_cc_library(
    name = "qst_engine",
    srcs = [
        "src/core/tracer_engine.cpp",
        "src/core/tracer_event_config.cpp",
        "src/core/tracer_ring_dump.cpp",
    ],
    hdrs = [
        "include/qst/core/tracer_engine.hpp",
        "include/qst/types.hpp",
    ],
    copts = ["-std=c++17", "-O2", "-Wall", "-Wextra"],
    includes = ["include"],
    defines = ["BAZEL_BUILD"] + _PLATFORM_DEFINES,
    deps = [
        ":qst_config",
        ":qst_data_buffer",
        ":qst_data",
        ":qst_log",
    ],
)

# ============================================================================
# QSchedTracer 主程序
# ============================================================================

platform_cc_binary(
    name = "qst_tracer_bin",
    srcs = ["src/main.cpp"],
    copts = ["-std=c++17", "-O2", "-Wall", "-Wextra"],
    includes = ["include"],
    defines = ["BAZEL_BUILD"] + _PLATFORM_DEFINES,
    deps = [
        ":qst_engine",
        ":qst_config",
        ":qst_log",
    ],
)

# ============================================================================
# 主程序包
# ============================================================================

deeproute_release_package(
    name = "qst_tracer_pkg",
    srcs = [":qst_tracer_bin"],
    mode = "0755",
    package_dir = "bin",
)

# ============================================================================
# 完整发布包
# ============================================================================

deeproute_release_package(
    name = "qst_tracer_release_package",
    mode = "0755",
    package_dir = "qst-tracer",
    deps = [
        ":qst_tracer_pkg",
    ],
)
