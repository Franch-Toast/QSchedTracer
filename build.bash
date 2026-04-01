#!/bin/bash
#==============================================================================
# QSchedTracer 编译脚本 - QNX 7.1 aarch64 (Ring Mode, QST v4)
#==============================================================================
#
# 用法:
#   ./build.bash              # 编译 QNX 7.1
#   ./build.bash clean        # 清理
#   ./build.bash debug        # 调试版本
#
# 输出:
#   build/qnx_aarch64/qst_tracer
#==============================================================================

set -e

QNX_HOST=/sandbox/toolchains_qnx/host/linux/x86_64
QNX_TARGET=/sandbox/toolchains_qnx/target/qnx7

CC=$QNX_HOST/usr/bin/aarch64-unknown-nto-qnx7.1.0-gcc-8.3.0
CXX=$QNX_HOST/usr/bin/aarch64-unknown-nto-qnx7.1.0-g++-8.3.0

SCRIPT_DIR=$(dirname "$(readlink -f "$0")")
BUILD_DIR="$SCRIPT_DIR/build/qnx_aarch64"
SRC_DIR="$SCRIPT_DIR/src"
INC_DIR="$SCRIPT_DIR/include"
SPDLOG_INC_DIR="$SCRIPT_DIR/spdlog/include"

CXXFLAGS="-D_QNX_SOURCE -D__QNXNTO__ -DLP8650 -std=c++17 -stdlib=libc++ -O2 -Wall -Wextra"
LDFLAGS="-Wl,--no-as-needed -lc++ -lm"

export QNX_HOST QNX_TARGET PATH=$QNX_HOST/usr/bin:$PATH CC CXX

if [ "$1" = "clean" ]; then
    rm -rf "$BUILD_DIR"
    echo "Clean done"
    exit 0
fi

if [ "$1" = "debug" ]; then
    CXXFLAGS="$CXXFLAGS -g -DDEBUG"
fi

if [ ! -d "$QNX_HOST" ]; then
    echo "Error: QNX toolchain not found: $QNX_HOST"
    exit 1
fi

mkdir -p "$BUILD_DIR"

echo ""
echo "QSchedTracer build - QNX 7.1 aarch64 (Ring Mode, QST v4)"
echo "========================================================="
echo ""

INC_FLAGS="-I$INC_DIR -I$SPDLOG_INC_DIR"

SOURCES=(
    "core/data_buffer.cpp"
    "core/tracer_engine.cpp"
    "data/data_manager.cpp"
    "main.cpp"
)

OBJECTS=""
TOTAL=${#SOURCES[@]}
COUNT=0

for src in "${SOURCES[@]}"; do
    COUNT=$((COUNT + 1))
    basename=$(basename "$src" .cpp)
    echo "[$COUNT/$TOTAL] Compiling $src..."
    $CXX $CXXFLAGS $INC_FLAGS -c "$SRC_DIR/$src" -o "$BUILD_DIR/${basename}.o"
    OBJECTS="$OBJECTS $BUILD_DIR/${basename}.o"
done

echo ""
echo "Linking qst_tracer..."
$CXX -stdlib=libc++ $LDFLAGS $OBJECTS -o "$BUILD_DIR/qst_tracer"

echo ""
echo "Build successful: $BUILD_DIR/qst_tracer"
file "$BUILD_DIR/qst_tracer"
ls -lh "$BUILD_DIR/qst_tracer"
echo ""
