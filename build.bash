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
#   - nlohmann/json 库已集成在项目的 third_party/nlohmann/ 目录
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
#   ./qst_tracer -c config.json
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
THIRD_PARTY_DIR="$SCRIPT_DIR/third_party"

# 编译标志
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

if [ ! -f "$THIRD_PARTY_DIR/nlohmann/json.hpp" ]; then
    echo "❌ 错误: nlohmann/json 未找到: $THIRD_PARTY_DIR/nlohmann/json.hpp"
    echo "   请将 nlohmann/json.hpp 放置在项目的 third_party/nlohmann/ 目录"
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
echo "║ 日志库: spdlog (header-only)"
echo "║ JSON库: nlohmann/json (header-only)"
echo "╚══════════════════════════════════════════════════════════╝"
echo ""

# Include 路径
INC_FLAGS="-I$INC_DIR -I$SPDLOG_INC_DIR -I$THIRD_PARTY_DIR"

# 源文件列表
SOURCES=(
    "core/data_buffer.cpp"
    "core/tracer_engine.cpp"
    "config/config_loader.cpp"
    "trigger/kernel_event_trigger.cpp"
    "trigger/trigger_manager.cpp"
    "event/event_manager.cpp"
    "data/data_manager.cpp"
    "main.cpp"
)

# 编译每个源文件
OBJECTS=""
TOTAL=${#SOURCES[@]}
COUNT=0

for src in "${SOURCES[@]}"; do
    COUNT=$((COUNT + 1))
    basename=$(basename "$src" .cpp)
    
    echo "[$COUNT/$TOTAL] 📦 编译 $src..."
    $CXX $CXXFLAGS $INC_FLAGS -c "$SRC_DIR/$src" -o "$BUILD_DIR/${basename}.o"
    
    OBJECTS="$OBJECTS $BUILD_DIR/${basename}.o"
done

# 链接
echo ""
echo "[链接] 🔗 生成 qst_tracer..."
$CXX -stdlib=libc++ $LDFLAGS $OBJECTS -o "$BUILD_DIR/qst_tracer"

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
echo "   ./qst_tracer                          # 使用默认配置"
echo "   ./qst_tracer -c config.json           # 使用指定配置"
echo "   ./qst_tracer -b 16                    # 16MB 缓冲"
echo ""
