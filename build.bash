#!/bin/bash
#==============================================================================
# QSchedTracer 编译脚本 - QNX aarch64 (Ring Mode, QST v4)
#==============================================================================
#
# 用法:
#   ./build.bash              # 编译 QNX 7.1 (默认)
#   ./build.bash qnx80       # 编译 QNX 8.0
#   ./build.bash clean        # 清理所有构建产物
#   ./build.bash debug        # QNX 7.1 调试版本
#   ./build.bash qnx80 debug # QNX 8.0 调试版本
#
# 输出:
#   build/qnx71_aarch64/qst_tracer   (QNX 7.1)
#   build/qnx80_aarch64/qst_tracer   (QNX 8.0)
#==============================================================================

set -e

SCRIPT_DIR=$(dirname "$(readlink -f "$0")")
SRC_DIR="$SCRIPT_DIR/src"
INC_DIR="$SCRIPT_DIR/include"
SPDLOG_INC_DIR="$SCRIPT_DIR/spdlog/include"

# Parse arguments
QNX_VER="71"
DEBUG_MODE=false
for arg in "$@"; do
    case "$arg" in
        qnx80)  QNX_VER="80" ;;
        debug)  DEBUG_MODE=true ;;
        clean)
            rm -rf "$SCRIPT_DIR/build/qnx71_aarch64" "$SCRIPT_DIR/build/qnx80_aarch64" \
                   "$SCRIPT_DIR/build/qnx_aarch64"
            echo "Clean done"
            exit 0
            ;;
        *)
            echo "Unknown argument: $arg"
            echo "Usage: $0 [qnx80] [debug|clean]"
            exit 1
            ;;
    esac
done

if [ "$QNX_VER" = "80" ]; then
    QNX_HOST=/sandbox/toolchains_qnx_sdp8/host/linux/x86_64
    QNX_TARGET=/sandbox/toolchains_qnx_sdp8/target/qnx
    CC=$QNX_HOST/usr/bin/aarch64-unknown-nto-qnx8.0.0-gcc
    CXX=$QNX_HOST/usr/bin/aarch64-unknown-nto-qnx8.0.0-g++
    PLATFORM_DEFINE="-DLP8797"
    BUILD_DIR="$SCRIPT_DIR/build/qnx80_aarch64"
    # QNX 8.0 GCC 12: -stdlib=libc++ adds -lc++abi which doesn't exist;
    # use -stdlib=libc++ only for compile, link explicitly with -lc++
    CXXFLAGS="-D_QNX_SOURCE -D__QNXNTO__ $PLATFORM_DEFINE -std=c++17 -stdlib=libc++ -O2 -Wall -Wextra"
    LDFLAGS="-Wl,--no-as-needed -lc++ -lm"
    LINK_STDLIB=""
    LABEL="QNX 8.0"
else
    QNX_HOST=/sandbox/toolchains_qnx/host/linux/x86_64
    QNX_TARGET=/sandbox/toolchains_qnx/target/qnx7
    CC=$QNX_HOST/usr/bin/aarch64-unknown-nto-qnx7.1.0-gcc-8.3.0
    CXX=$QNX_HOST/usr/bin/aarch64-unknown-nto-qnx7.1.0-g++-8.3.0
    PLATFORM_DEFINE="-DLP8650"
    BUILD_DIR="$SCRIPT_DIR/build/qnx71_aarch64"
    CXXFLAGS="-D_QNX_SOURCE -D__QNXNTO__ $PLATFORM_DEFINE -std=c++17 -stdlib=libc++ -O2 -Wall -Wextra"
    LDFLAGS="-Wl,--no-as-needed -lc++ -lm"
    LINK_STDLIB="-stdlib=libc++"
    LABEL="QNX 7.1"
fi

if $DEBUG_MODE; then
    CXXFLAGS="$CXXFLAGS -g -DDEBUG"
fi

export QNX_HOST QNX_TARGET PATH=$QNX_HOST/usr/bin:$PATH CC CXX

if [ ! -d "$QNX_HOST" ]; then
    echo "Error: QNX toolchain not found: $QNX_HOST"
    exit 1
fi

mkdir -p "$BUILD_DIR"

echo ""
echo "QSchedTracer build - $LABEL aarch64 (Ring Mode, QST v4)"
echo "========================================================="
echo ""

INC_FLAGS="-I$INC_DIR -I$SPDLOG_INC_DIR"

SOURCES=(
    "core/tracer_engine.cpp"
    "core/tracer_event_config.cpp"
    "core/tracer_ring_dump.cpp"
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
$CXX $LINK_STDLIB $LDFLAGS $OBJECTS -o "$BUILD_DIR/qst_tracer"

echo ""
echo "Build successful: $BUILD_DIR/qst_tracer"
file "$BUILD_DIR/qst_tracer"
ls -lh "$BUILD_DIR/qst_tracer"
echo ""
