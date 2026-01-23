workspace(name = "qst")

load("@bazel_tools//tools/build_defs/repo:git.bzl", "git_repository", "new_git_repository")
load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

# ============================================================================
# DeepRoute Build Tools (Bazel 配置和工具链)
# ============================================================================

local_repository(
    name = "deeproute_build_tools",
    path = "../bazel_configs",
)

# ============================================================================
# Third Party 依赖 (包含 spdlog 等)
# ============================================================================

local_repository(
    name = "third_party",
    path = "../third_party",
)

# 加载 third_party 依赖 (包括 spdlog)
load("@third_party//:dr_third_party.bzl", "declare_dr_third_party_repositories")
declare_dr_third_party_repositories()

load("@third_party//:dr_third_party_extra.bzl", "declare_dr_third_party_repositories_extra")
declare_dr_third_party_repositories_extra()
