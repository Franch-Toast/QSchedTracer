load("@deeproute_build_tools//rules/modules:platform_cc_wrapper.bzl", "platform_cc_library", "platform_cc_binary")
load("@deeproute_build_tools//rules/pkg:deeproute_release.bzl", "deeproute_release_package")
load("@deeproute_build_tools//toolchains:defs.bzl", "deeproute_register_toolchains")

package(default_visibility = ["//visibility:public"])

# ============================================================================
# 注册工具链 (支持 QNX 交叉编译)
# ============================================================================

deeproute_register_toolchains()

# ============================================================================
# QSchedTracer 日志适配层
# - Bazel 编译时定义 BAZEL_BUILD 宏，使用 @spdlog (compiled library)
# - 脚本/CMake 编译时使用本地 spdlog (header-only)
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
# nlohmann/json 库 (header-only)
# ============================================================================

platform_cc_library(
    name = "nlohmann_json",
    hdrs = ["third_party/nlohmann/json.hpp"],
    includes = ["third_party"],
    visibility = ["//visibility:private"],
)

# ============================================================================
# QSchedTracer 核心库 - 数据缓冲区
# ============================================================================

platform_cc_library(
    name = "qst_data_buffer",
    srcs = ["src/core/data_buffer.cpp"],
    hdrs = [
        "include/qst/core/data_buffer.hpp",
        "include/qst/types.hpp",
    ],
    copts = [
        "-std=c++17",
        "-O2",
        "-Wall",
        "-Wextra",
    ],
    includes = ["include"],
    defines = ["BAZEL_BUILD"],
    deps = [":qst_log"],
)

# ============================================================================
# QSchedTracer 配置模块
# ============================================================================

platform_cc_library(
    name = "qst_config",
    srcs = ["src/config/config_loader.cpp"],
    hdrs = [
        "include/qst/config/types.hpp",
        "include/qst/config/config_loader.hpp",
    ],
    copts = [
        "-std=c++17",
        "-O2",
        "-Wall",
        "-Wextra",
    ],
    includes = ["include"],
    defines = ["BAZEL_BUILD"],
    deps = [
        ":qst_log",
        ":nlohmann_json",
    ],
)

# ============================================================================
# QSchedTracer 触发器模块
# ============================================================================

platform_cc_library(
    name = "qst_trigger",
    srcs = [
        "src/trigger/kernel_event_trigger.cpp",
        "src/trigger/trigger_manager.cpp",
    ],
    hdrs = [
        "include/qst/trigger/trigger.hpp",
        "include/qst/trigger/kernel_event_trigger.hpp",
        "include/qst/trigger/trigger_manager.hpp",
    ],
    copts = [
        "-std=c++17",
        "-O2",
        "-Wall",
        "-Wextra",
    ],
    includes = ["include"],
    defines = ["BAZEL_BUILD"],
    deps = [
        ":qst_config",
        ":qst_log",
    ],
)

# ============================================================================
# QSchedTracer 事件模块
# ============================================================================

platform_cc_library(
    name = "qst_event",
    srcs = ["src/event/event_manager.cpp"],
    hdrs = ["include/qst/event/event_manager.hpp"],
    copts = [
        "-std=c++17",
        "-O2",
        "-Wall",
        "-Wextra",
    ],
    includes = ["include"],
    defines = ["BAZEL_BUILD"],
    deps = [
        ":qst_config",
        ":qst_log",
    ],
)

# ============================================================================
# QSchedTracer 数据模块
# ============================================================================

platform_cc_library(
    name = "qst_data",
    srcs = ["src/data/data_manager.cpp"],
    hdrs = ["include/qst/data/data_manager.hpp"],
    copts = [
        "-std=c++17",
        "-O2",
        "-Wall",
        "-Wextra",
    ],
    includes = ["include"],
    defines = ["BAZEL_BUILD"],
    deps = [
        ":qst_data_buffer",
        ":qst_log",
    ],
)

# ============================================================================
# QSchedTracer 核心引擎
# ============================================================================

platform_cc_library(
    name = "qst_engine",
    srcs = ["src/core/tracer_engine.cpp"],
    hdrs = [
        "include/qst/core/tracer_engine.hpp",
        "include/qst/types.hpp",
    ],
    copts = [
        "-std=c++17",
        "-O2",
        "-Wall",
        "-Wextra",
    ],
    includes = ["include"],
    defines = ["BAZEL_BUILD"],
    deps = [
        ":qst_config",
        ":qst_data_buffer",
        ":qst_event",
        ":qst_trigger",
        ":qst_data",
        ":qst_log",
    ],
)

# ============================================================================
# QSchedTracer - QNX 调度追踪器主程序
# ============================================================================

platform_cc_binary(
    name = "qst_tracer_bin",
    srcs = ["src/main.cpp"],
    copts = [
        "-std=c++17",
        "-O2",
        "-Wall",
        "-Wextra",
    ],
    includes = ["include"],
    defines = ["BAZEL_BUILD"],
    deps = [
        ":qst_engine",
        ":qst_config",
        ":qst_log",
    ],
)

# ============================================================================
# 工具包 - 包含解析器脚本
# ============================================================================

deeproute_release_package(
    name = "qst_tools",
    srcs = [
        "tools/qst_parse.py",
    ],
    mode = "0755",
    package_dir = "tools",
)

# ============================================================================
# 配置包
# ============================================================================

deeproute_release_package(
    name = "qst_config_pkg",
    srcs = [
        "config/default.json",
    ],
    mode = "0644",
    package_dir = "etc/qst",
)

# ============================================================================
# 主程序包
# ============================================================================

deeproute_release_package(
    name = "qst_tracer_pkg",
    srcs = [
        ":qst_tracer_bin",
    ],
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
        ":qst_tools",
        ":qst_config_pkg",
    ],
)
