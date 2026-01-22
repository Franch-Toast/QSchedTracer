#!/bin/bash
#==============================================================================
# QSchedTracer 编译脚本 - QNX 7.1 aarch64
#==============================================================================
#
# 描述:
#   此脚本用于交叉编译 QSchedTracer 项目，生成可在 QNX 7.1 aarch64 平台
#   运行的可执行文件。
#
# 前提条件:
#   - QNX 7.1 SDP 工具链已安装在 /sandbox/toolchains_qnx/
#   - spdlog 日志库已集成在项目的 spdlog/ 目录
#
# 用法:
#   ./build.bash              # 编译项目
#   ./build.bash clean        # 清理构建目录
#   ./build.bash debug        # 编译调试版本 (带调试符号)
#
# 输出:
#   build/qnx_aarch64/qst_tracer    # QNX ARM64 可执行文件
#
# 部署:
#   scp build/qnx_aarch64/qst_tracer target:/path/to/
#   # 在 QNX 目标机上:
#   ./qst_tracer -d 10 -o trace.qst
#
#==============================================================================

set -e

# ============================================================================
# 配置区域
# ============================================================================

# QNX 工具链路径
QNX_HOST=/sandbox/toolchains_qnx/host/linux/x86_64
QNX_TARGET=/sandbox/toolchains_qnx/target/qnx7

# 编译器
CC=$QNX_HOST/usr/bin/aarch64-unknown-nto-qnx7.1.0-gcc-8.3.0
CXX=$QNX_HOST/usr/bin/aarch64-unknown-nto-qnx7.1.0-g++-8.3.0

# 目录设置
SCRIPT_DIR=$(dirname "$(readlink -f "$0")")
BUILD_DIR="$SCRIPT_DIR/build/qnx_aarch64"
SRC_DIR="$SCRIPT_DIR/src"
INC_DIR="$SCRIPT_DIR/include"
SPDLOG_INC_DIR="$SCRIPT_DIR/spdlog/include"

# 编译标志
# -D_QNX_SOURCE      : 启用 QNX 特定 API
# -D__QNXNTO__       : 标识 QNX Neutrino 平台
# -std=c++17         : 使用 C++17 标准
# -stdlib=libc++     : 使用 libc++ 标准库 (QNX 默认)
# -O2                : 优化级别 2
# -Wall -Wextra      : 启用警告
CXXFLAGS="-D_QNX_SOURCE -D__QNXNTO__ -std=c++17 -stdlib=libc++ -O2 -Wall -Wextra"

# 链接标志
LDFLAGS="-Wl,--no-as-needed -lc++ -lm"

# ============================================================================
# 环境设置
# ============================================================================

export QNX_HOST
export QNX_TARGET
export PATH=$QNX_HOST/usr/bin:$PATH
export CC
export CXX

# ============================================================================
# 命令处理
# ============================================================================

# 清理命令
if [ "$1" = "clean" ]; then
    echo "🧹 清理构建目录..."
    rm -rf "$BUILD_DIR"
    echo "✅ 清理完成"
    exit 0
fi

# 调试版本
if [ "$1" = "debug" ]; then
    CXXFLAGS="$CXXFLAGS -g -DDEBUG"
    echo "🔧 编译调试版本..."
fi

# ============================================================================
# 检查依赖
# ============================================================================

if [ ! -d "$QNX_HOST" ]; then
    echo "❌ 错误: QNX 工具链未找到: $QNX_HOST"
    echo "   请确保 QNX 7.1 SDP 已正确安装"
    exit 1
fi

if [ ! -d "$SPDLOG_INC_DIR" ]; then
    echo "❌ 错误: spdlog 未找到: $SPDLOG_INC_DIR"
    echo "   请将 spdlog 库放置在项目的 spdlog/ 目录"
    exit 1
fi

# ============================================================================
# 编译
# ============================================================================

# 创建构建目录
mkdir -p "$BUILD_DIR"

echo ""
echo "╔══════════════════════════════════════════════════════════╗"
echo "║       QSchedTracer 编译 - QNX 7.1 aarch64                ║"
echo "╠══════════════════════════════════════════════════════════╣"
echo "║ 编译器: $(basename $CXX)"
echo "║ C++标准: C++17"
echo "║ 日志库: spdlog 1.17.0 (header-only)"
echo "╚══════════════════════════════════════════════════════════╝"
echo ""

# Include 路径
INC_FLAGS="-I$INC_DIR -I$SPDLOG_INC_DIR"

# 编译源文件
echo "[1/3] 📦 编译 ring_buffer.cpp..."
$CXX $CXXFLAGS $INC_FLAGS -c "$SRC_DIR/ring_buffer.cpp" -o "$BUILD_DIR/ring_buffer.o"

echo "[2/3] 📦 编译 tracer.cpp..."
$CXX $CXXFLAGS $INC_FLAGS -c "$SRC_DIR/tracer.cpp" -o "$BUILD_DIR/tracer.o"

echo "[3/3] 📦 编译 main.cpp..."
$CXX $CXXFLAGS $INC_FLAGS -c "$SRC_DIR/main.cpp" -o "$BUILD_DIR/main.o"

# 链接
echo ""
echo "[链接] 🔗 生成 qst_tracer..."
$CXX -stdlib=libc++ $LDFLAGS \
    "$BUILD_DIR/ring_buffer.o" \
    "$BUILD_DIR/tracer.o" \
    "$BUILD_DIR/main.o" \
    -o "$BUILD_DIR/qst_tracer"

# ============================================================================
# 完成
# ============================================================================

echo ""
echo "╔══════════════════════════════════════════════════════════╗"
echo "║                    ✅ 编译成功!                          ║"
echo "╠══════════════════════════════════════════════════════════╣"
echo "║ 输出文件: $BUILD_DIR/qst_tracer"
echo "╚══════════════════════════════════════════════════════════╝"
echo ""

# 显示文件信息
file "$BUILD_DIR/qst_tracer"
ls -lh "$BUILD_DIR/qst_tracer"

echo ""
echo "📋 部署命令:"
echo "   scp $BUILD_DIR/qst_tracer target:/path/to/"
echo ""
echo "📋 运行命令 (在 QNX 目标机上):"
echo "   ./qst_tracer -d 10 -o trace.qst"
echo ""
