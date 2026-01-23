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

platform_cc_library(
    name = "local_spdlog",
    hdrs = glob([
        "spdlog/include/spdlog/*.h",
        "spdlog/include/spdlog/**/*.h",
    ]),
    includes = [
        "spdlog/include",
    ],
    strip_include_prefix = "spdlog/include",
    visibility = ["//visibility:private"],
)

# ============================================================================
# QSchedTracer 核心库
# ============================================================================

platform_cc_library(
    name = "qst_ring_buffer",
    srcs = ["src/ring_buffer.cpp"],
    hdrs = [
        "include/qst/ring_buffer.hpp",
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

platform_cc_library(
    name = "qst_tracer",
    srcs = ["src/tracer.cpp"],
    hdrs = [
        "include/qst/tracer.hpp",
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
        ":qst_ring_buffer",
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
        ":qst_tracer",
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
    ],
)
