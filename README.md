# QSchedTracer

**QNX 调度追踪器** - 轻量级飞行记录仪模式调度事件采集工具

## 概述

QSchedTracer 是专为 QNX7.1 Neutrino RTOS 设计的调度事件追踪工具。采用"飞行记录仪"模式持续记录系统调度事件，支持导出为 [Perfetto](https://ui.perfetto.dev/) 可视化格式。

**主要特性**：

- 高性能采集：`InterruptHookTrace` + Linear 模式，中断级数据传输
- 飞行记录仪模式：环形缓冲区持续记录，仅在需要时落盘
- 真实时间戳：支持 UNIX 时间戳转换，精确到微秒
- Perfetto 集成：导出标准 Chrome Trace Event 格式

## 快速开始

```bash
# 1. 编译
./build.bash (或使用cmake编译)

# 2. 部署到 QNX 设备
scp build/qnx_aarch64/qst_tracer root@<QNX_IP>:/tmp/

# 3. 运行采集 (5秒)
ssh root@<QNX_IP> '/tmp/qst_tracer -d 5 -o /tmp/trace.qst'

# 4. 取回数据
scp root@<QNX_IP>:/tmp/trace.qst .

# 5. 解析并导出 JSON
python3 tools/qst_parse.py trace.qst -o trace.json

# 6. 可视化
   打开 https://ui.perfetto.dev/ 并拖入 trace.json
```

## 架构

```
┌─────────────────────────────────────────────────────────────────────┐
│                          QNX 内核                                   │
│                                                                     │
│  调度器事件 ──▶ traceevent_t (16 bytes)                             │
│     │           - header: CPU, 事件类型                             │
│     │           - data[0]: 时间戳 (32-bit cycles)                   │
│     │           - data[1-2]: pid, tid, state                        │
│     ▼                                                               │
│  内核 Trace Buffer (tracebuf_t × 16, Linear 模式)                   │
│     │                                                               │
│     │ Buffer 满时触发 InterruptHookTrace                            │
└─────┼───────────────────────────────────────────────────────────────┘
      │
      │ 中断回调 (memcpy ~16KB, ~2-4µs)
      ▼
┌─────────────────────────────────────────────────────────────────────┐
│                     QSchedTracer 进程                               │
│                                                                     │
│  RingBuffer (10MB, 环形覆盖)                                        │
│     │  - 自动覆盖旧数据，保留最近 N 秒                              │
│     │                                                               │
│     │ 停止采集 (Ctrl+C / 超时)                                      │
│     ▼                                                               │
│  1. _NTO_TRACE_STOP + FLUSHBUFFER                                   │
│  2. 记录真实时间同步点 (wallclock + cycles)                         │
│  3. 使用独立缓冲区采集进程/线程名称                                 │
│  4. 写入 .qst 文件                                                  │
└─────────────────────────────────────────────────────────────────────┘
      │
      ▼
┌─────────────────────────────────────────────────────────────────────┐
│                       离线处理                                      │
│                                                                     │
│  trace.qst ──▶ qst_parse.py ──▶ trace.json ──▶ Perfetto UI          │
└─────────────────────────────────────────────────────────────────────┘
```

## 文件格式

### .qst v3 格式

```
┌──────────────────────────────────────────────────────────────┐
│ FileHeader (64 字节)                                         │
│   - magic: 0x51535433 ('QST3')                               │
│   - clock_freq: 时钟频率 (Hz)                                │
│   - wallclock_sec/nsec: 采集结束时的系统时间                 │
│   - sync_cycles: 最后一个事件的 cycles                       │
├──────────────────────────────────────────────────────────────┤
│ ProcInfoHeader (16 字节)                                     │
│   - magic: 0x50494E46 ('PINF')                               │
│   - event_count: 事件数量                                    │
├──────────────────────────────────────────────────────────────┤
│ ProcInfo 数据 (进程/线程名称)                                │
├──────────────────────────────────────────────────────────────┤
│ MainDataHeader (16 字节)                                     │
│   - magic: 0x4D41494E ('MAIN')                               │
│   - event_count: 事件数量                                    │
├──────────────────────────────────────────────────────────────┤
│ 主调度数据 (按时间顺序写入)                                  │
└──────────────────────────────────────────────────────────────┘
```

## 编译

### 环境要求

| 依赖 | 版本 | 说明 |
|------|------|------|
| QNX SDP | 7.1+ | 交叉编译工具链 |
| C++ | C++17 | 语言标准 |
| spdlog | 1.15+ | 日志库 (自动选择，见下文) |
| Python | 3.6+ | 解析器运行环境 |

### 日志库说明

QSchedTracer 支持两种 spdlog 来源，通过编译方式自动选择：

| 编译方式 | spdlog 来源 | 宏定义 |
|----------|-------------|--------|
| Bazel | `@spdlog//:spdlog` (compiled library) | `BAZEL_BUILD` |
| build.bash / CMake | 本地 `spdlog/` (header-only) | 无 |

代码层面使用统一接口 `LOG_INFO(fmt, ...)`，无需修改。详见 `include/qst/log.hpp`。

### 方法一：使用 build.bash (推荐)

```bash
# 编译 Release 版本
./build.bash

# 编译 Debug 版本
./build.bash debug

# 清理
./build.bash clean
```

**输出**：`build/qnx_aarch64/qst_tracer`

### 方法二：使用 CMake

```bash
# 1. 设置 QNX 环境
export QNX_HOST=/sandbox/toolchains_qnx/host/linux/x86_64
export QNX_TARGET=/sandbox/toolchains_qnx/target/qnx7
export PATH=/tmp:$QNX_HOST/usr/bin:$PATH  # 创建ld符号链接

# 2. 创建构建目录
mkdir cmake_build && cd cmake_build

# 3. 配置 (aarch64)
cmake -DCMAKE_TOOLCHAIN_FILE=../cmake/qnx710-aarch64.cmake ..

# 4. 编译
make
```

**输出**：`qst_tracer` (QNX ARM64 可执行文件)

### 方法三：使用 Bazel 构建 (推荐用于 CI/CD)

该方案需要 bazel_configs 和 third_party 仓库，可以使用提供的 BUILD 文件进行构建：

```bash
# 构建可执行文件 (QNX SA8650 平台)
bazel build --config=sa8650_qnx //:qst_tracer_bin

# 构建完整发布包
bazel build --config=sa8650_qnx //:qst_tracer_release_package

```

#### Bazel 平台配置

| 配置 | 说明 | 命令示例 |
|------|------|----------|
| `sa8650_qnx` | SA8650 QNX aarch64 平台 | `bazel build --config=sa8650_qnx //:qst_tracer_bin` |
| `sa8797_qnx` | SA8797 QNX aarch64 平台 | `bazel build --config=sa8797_qnx //:qst_tracer_bin` |


#### Bazel 依赖说明

BUILD 文件使用了 `platform_cc_library` 和 `platform_cc_binary` 宏，这些宏来自 `bazel_configs/rules/modules/platform_cc_wrapper.bzl`，提供了：
- QNX 工具链自动注册 (`deeproute_register_toolchains`)
- 平台特定的编译选项
- 跨平台构建支持


### 方法四：使用 Bear 生成 compile_commands.json (IDE 支持)

```bash
# 1. 安装 bear (Ubuntu/Debian)
sudo apt install bear

# 2. 使用 bear 包装编译脚本
bear -- ./build.bash
```

这将生成 `compile_commands.json` 文件，配合 clangd 扩展可实现代码跳转、自动补全等功能。

**VS Code 配置**：
1. 安装 clangd 扩展
2. 项目已包含 `.vscode/settings.json`，会自动使用生成的 `compile_commands.json`

> ⚠️ 注意：需要先配置 QNX 环境变量

```bash
# 1. 设置 QNX 环境
source /path/to/qnx710/qnxsdp-env.sh

# 2. 创建构建目录
mkdir build && cd build

# 3. 配置
cmake -DCMAKE_TOOLCHAIN_FILE=../cmake/qnx710-aarch64.cmake ..

# 4. 编译
make
```

### 编译选项说明

| 标志 | 说明 |
|------|------|
| `-D_QNX_SOURCE` | 启用 QNX 特定 API |
| `-D__QNXNTO__` | 标识 QNX Neutrino 平台 |
| `-std=c++17` | 使用 C++17 标准 |
| `-stdlib=libc++` | 使用 libc++ 标准库 |
| `-O2` | 优化级别 2 |

## 使用方法

### 采集器命令行

```
qst_tracer [选项]

选项:
  -d <秒>    采集时长 (默认: 5, 0=无限制，Ctrl+C 停止)
  -o <文件>  输出文件 (默认: trace.qst)
  -b <MB>    缓冲区大小 (默认: 10)
  -v         详细输出
  -h         显示帮助

示例:
  qst_tracer -d 10 -o trace.qst      # 采集 10 秒
  qst_tracer -d 0 -b 20              # 20MB 缓冲，无限采集
```

### 解析器命令行

```bash
# 显示摘要信息
python3 tools/qst_parse.py trace.qst

# 导出 Perfetto JSON
python3 tools/qst_parse.py trace.qst -o trace.json

# 详细输出
python3 tools/qst_parse.py trace.qst -o trace.json -v
```

### 可视化

1. 打开 https://ui.perfetto.dev/
2. 拖拽 `trace.json` 到页面
3. 查看调度时间线

## 项目结构

```
QSchedTracer/
├── build.bash                 # 编译脚本 (推荐使用)
├── CMakeLists.txt             # CMake 构建配置
├── README.md                  # 本文档
│
├── cmake/
│   ├── qnx710-aarch64.cmake   # QNX aarch64 工具链
│   └── qnx710-x86_64.cmake    # QNX x86_64 工具链
│
├── include/qst/
│   ├── types.hpp              # 类型定义 (QstEvent, FileHeader 等)
│   ├── ring_buffer.hpp        # 环形缓冲区类
│   ├── tracer.hpp             # 追踪器主类
│   └── log.hpp                # 日志工具 (基于 spdlog)
│
├── src/
│   ├── ring_buffer.cpp        # 环形缓冲区实现
│   ├── tracer.cpp             # 追踪器实现 (QNX trace API)
│   └── main.cpp               # 主程序入口
│
├── spdlog/                    # spdlog 1.17.0 (header-only)
│   └── include/spdlog/
│
├── tools/
│   └── qst_parse.py           # Python 解析器
│
├── docs/
│   └── trace_event_format.md  # Perfetto 格式参考
│
└── build/                     # 构建输出目录
    └── qnx_aarch64/
        └── qst_tracer         # 可执行文件
```

## 技术细节

### 时间戳处理

QNX trace 事件使用 32 位 cycles 时间戳，约 3-4 秒回绕一次。QSchedTracer v3 采用"结束同步点"方案：

1. 采集结束时记录 `wallclock` (CLOCK_REALTIME) + `sync_cycles` (最后事件的 cycles)
2. 解析器从最后一个事件向前推算，自动处理回绕
3. 真实时间 = wallclock - (sync_cycles - event_cycles) / clock_freq

### 进程/线程名称采集

飞行记录仪模式下，初始进程信息会被覆盖。解决方案：

1. 主采集使用 `_NTO_TRACE_STARTNOSTATE` (不注入初始状态)
2. 落盘前使用**独立缓冲区**调用 `_NTO_TRACE_START` 采集名称
3. 通过 `active_buffer_` 指针动态切换，避免污染主调度数据

### 性能特性

| 指标 | 数值 |
|------|------|
| 中断处理时间 | ~2-4 µs (memcpy 16KB) |
| 中断频率 | ~100-500 Hz |
| CPU 开销 | < 0.2% |
| 事件容量 | ~625K 事件/10MB |

## 参考文献

1. [Trace Event Format](https://docs.google.com/document/u/0/d/1CvAClvFfyA5R-PhYUmn5OOQtYMH4h6I0nSsKchNAySU/mobilebasic?tab=t.0&_immersive_translate_auto_translate=1#heading=h.yr4qxyxotyw)

## License

MIT License
