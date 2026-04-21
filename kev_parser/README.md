# kev_parser

QNX `.kev` 二进制内核追踪文件 → 结构化 JSONL / 文本 转换器。

基于 QNX 官方 `libtraceparser` 库，将 `tracelogger` 产出的 `.kev` 文件解码为 AI 和人类均可直接消费的格式。自动处理环形缓冲区间隙检测、墙钟时间映射和 PID/TID 名称解析。

支持 QNX 7.1 和 QNX 8.0 两个版本，线程状态/内核调用/IPC 通信事件解析结果与官方 `traceprinter` 100% 对齐。

## 项目结构

```
kev_parser/
├── CMakeLists.txt          # 构建配置
├── README.md
├── deps/                   # 内置依赖（无需外部 toolchain 即可编译）
│   ├── qnx71/lib/libtraceparser.a   # QNX 7.1 x86_64 静态库
│   └── qnx80/lib/libtraceparser.a   # QNX 8.0 x86_64 静态库
├── include/kev/             # 头文件
│   ├── qnx_compat.h        # Linux host 的 QNX API 兼容层
│   ├── kev_parser.h         # 主解析器
│   ├── event_names.h        # 事件名称映射
│   └── ...
├── src/                     # 源文件
├── cmake/                   # QNX 交叉编译工具链文件
└── docs/                    # 设计文档
```

## 快速开始

```bash
# 编译 QNX 8.0 版本
cmake -B build -DQNX_VERSION=80
cmake --build build -j$(nproc)

# 解析 kev 文件（输出 JSONL）
./build/kev_parser trace.kev

# 仅输出统计摘要
./build/kev_parser trace.kev -m summary -o -

# 最后 5 秒事件，文本格式
./build/kev_parser trace.kev --time-start -5 -f text -o -
```

## 编译

### 前置依赖

- CMake >= 3.14
- GCC / Clang（C++17）
- `libtraceparser.a`（已内置于 `deps/` 目录，无需额外安装）

### Linux Host 编译（推荐）

在 Linux x86_64 上编译，静态链接 QNX 的 `libtraceparser.a`。通过 `qnx_shim.c` 提供 QNX 特有符号的兼容实现。**项目自带所需的 `libtraceparser.a`，无需安装 QNX 工具链。**

```bash
# QNX 7.1 版本（解析 QNX 7.1 产生的 .kev 文件）
cmake -B build_71 -DQNX_VERSION=71
cmake --build build_71 -j$(nproc)

# QNX 8.0 版本（解析 QNX 8.0 产生的 .kev 文件）
cmake -B build_80 -DQNX_VERSION=80
cmake --build build_80 -j$(nproc)
```

`libtraceparser.a` 查找顺序：
1. QNX 工具链目录（如果设置了 `QNX71_TOOLCHAIN_ROOT` / `QNX80_TOOLCHAIN_ROOT`）
2. 项目平级目录 `../toolchains_qnx` / `../toolchains_qnx_sdp8`
3. `/sandbox/toolchains_qnx` / `/sandbox/toolchains_qnx_sdp8`
4. **项目内置 `deps/qnx71/lib/` / `deps/qnx80/lib/`**（默认回退）

### QNX 交叉编译

生成可在 QNX 目标板上直接运行的二进制（需要完整 QNX 工具链）：

```bash
# QNX 7.1 aarch64
cmake -B build_qnx71 -DCMAKE_TOOLCHAIN_FILE=cmake/qnx710-aarch64.cmake
cmake --build build_qnx71 -j$(nproc)

# QNX 8.0 aarch64
cmake -B build_qnx80 -DCMAKE_TOOLCHAIN_FILE=cmake/qnx800-aarch64.cmake
cmake --build build_qnx80 -j$(nproc)
```

### 版本限制

QNX 8.0 的 `libtraceparser` 无法解析 QNX 7.1 产生的 `.kev` 文件（反之亦然）。必须使用版本匹配的库：
- `-DQNX_VERSION=71` → 解析 QNX 7.1 的 `.kev` 文件
- `-DQNX_VERSION=80` → 解析 QNX 8.0 的 `.kev` 文件

## 使用

运行 `kev_parser --help` 可查看完整帮助信息。

### 命令行参数

| 选项 | 说明 |
|------|------|
| `-o PATH` | 输出文件（默认: `<input>.jsonl`，`-` 表示 stdout） |
| `-f FORMAT` | 输出格式：`jsonl`（默认）或 `text` |
| `-m MODE` | 输出模式：`full`（默认）或 `summary`（仅统计） |
| `-v, --verbose` | 详细 stderr 输出 |
| `--time-start T` | 开始时间过滤 |
| `--time-end T` | 结束时间过滤 |
| `--pid PID,...` | 按进程 ID 过滤（逗号分隔） |
| `--tid TID,...` | 按线程 ID 过滤（逗号分隔） |
| `--class CLS,...` | 按事件类过滤：`THREAD`, `KERCALL_ENTER`, `KERCALL_EXIT`, `COMM`, `SYSTEM`, `INT`, `PROCESS` |

### 时间格式

| 格式 | 说明 |
|------|------|
| `+N.nnn` | 从追踪开始向后的相对秒数 |
| `-N.nnn` | 从追踪结束向前的相对秒数 |
| `YYYY-MM-DD HH:MM:SS[.frac]` | 绝对墙钟时间（UTC），需文件名含时间戳 |
| `NNN` | 原始 trace 纳秒值 |

### 使用示例

```bash
# 完整解析 (输出 .jsonl 文件)
kev_parser tracer.log.10400.20260319.135220.kev

# 输出到 stdout 管道处理
kev_parser trace.kev -o - | head -20

# 墙钟时间范围过滤
kev_parser trace.kev \
    --time-start "2026-03-19 13:53:00" \
    --time-end "2026-03-19 13:54:00"

# PID + 事件类过滤
kev_parser trace.kev --pid 151588 --class KERCALL_ENTER,COMM -o -

# 组合过滤 + 文本格式
kev_parser trace.kev --pid 151588 --time-start -5 -f text -o -

# 仅查看摘要统计
kev_parser trace.kev -m summary -o -
```

## 输出格式

### JSONL 模式（默认）

每行一个 JSON 对象，按顺序输出四类消息：

**metadata** — 文件和追踪元信息（第一行）：
```json
{"type":"metadata","file_info":{"qnx_version":"8.0.0","cpu_count":18,"cycles_per_sec":19200000,...}}
```

**process / thread_name** — PID/TID 名称注册：
```json
{"type":"process","pid":1,"name":"proc/boot/procnto-smp-instr","parent_pid":0}
{"type":"thread_name","pid":1,"tid":1,"name":"idle_cpu_0"}
```

**event** — 事件流（按时间顺序）：
```json
{"type":"event","seq":1,"time_ns":618692347442,"cpu":4,"class":"THREAD","event":"RUNNING","pid":159334538,"tid":15,"process":"dr_sensor_lidar","data":{"priority":"10","policy":"1"}}
```

**summary** — 统计摘要（最后一行）：
```json
{"type":"summary","total_events":14777459,"duration_s":"30.287","thread_state_distribution":{"RUNNING":3763386,"READY":3745698,...},"top_kercalls":[...],"processes":[...]}
```

### Text 模式 (`-f text`)

```
=== QNX Kernel Trace ===
QNX 8.0.0 | SA8797P_v2.0... | 18 CPUs @ 19200000 Hz

  PROCESS  pid=1       proc/boot/procnto-smp-instr
  THREAD   pid=1      tid=1     idle_cpu_0

[ 57.512357340] CPU1 KERCALL_ENTER  TimerTimeout  pid=16388/1  clock_id=0x00000000

--- Summary ---
Total events: 14777459 | Duration: 30.288 s
```

## 支持的事件类型

### 线程状态 (THREAD)

| QNX 7.1 | QNX 8.0 | 说明 |
|---------|---------|------|
| DEAD | DEAD | 线程已终止 |
| RUNNING | RUNNING | 正在 CPU 上执行 |
| READY | READY | 就绪等待调度 |
| STOPPED | STOPPED | 被调试器暂停 |
| SEND | SEND | 阻塞在 MsgSend |
| RECEIVE | RECEIVE | 阻塞在 MsgReceive |
| REPLY | REPLY | 等待 MsgReply |
| STACK | MQ_SEND | 7.1: 等待栈; 8.0: POSIX mq_send |
| WAITTHREAD | MQ_RECEIVE | 7.1: 等待线程; 8.0: POSIX mq_receive |
| NANOSLEEP | NANOSLEEP | 睡眠 |
| MUTEX | MUTEX | 阻塞在互斥锁 |
| CONDVAR | CONDVAR | 阻塞在条件变量 |
| INTR | INTR | 等待中断 |
| SEM | SEM | 阻塞在信号量 |
| NET_SEND | RWLOCK_READ | 7.1: 网络 send; 8.0: 读写锁读 |
| NET_REPLY | RWLOCK_WRITE | 7.1: 网络 reply; 8.0: 读写锁写 |
| — | MUON_MUTEX | QNX 8.0 独有 |
| — | BARRIER | QNX 8.0 独有 |
| CREATE | CREATE | 线程创建 |
| DESTROY | DESTROY | 线程销毁 |

### 内核调用 (KERCALL)

支持 100+ 内核调用的语义化参数解析，包括：
MsgSendv, MsgReceivev, MsgReplyv, SyncMutexLock, SyncCondvarWait, TimerTimeout, SignalKill, InterruptWait, ChannelCreate 等。

### IPC 通信 (COMM)

SND_MSG, RCV_MSG, REPLY_MSG, SND_PULSE_*, RCV_PULSE_*, SND_SIGNAL, RCV_SIGNAL, ERROR

## 架构

```
               ┌──────────────┐
               │ .kev 文件     │
               └──────┬───────┘
                      │
               ┌──────▼───────┐
               │libtraceparser │  QNX 官方 C 解析库
               │(header/combine│  处理 combine 重组、时间戳重建、字节序
               │ /timestamp)   │
               └──────┬───────┘
                      │ callbacks
         ┌────────────▼────────────┐
         │       KevParser         │  主解析器
         │                         │
         │  NameTracker            │  pid/tid → 进程/线程名映射
         │  EventNames             │  内核调用/状态/通信事件名
         │  KercallFields          │  92+ 内核调用参数语义化命名
         │  RawKevTimestamps       │  CLK 失败时的原始时间戳重建
         │  EventFilter            │  时间/PID/TID/class 过滤
         │  StatsSummary           │  按进程/状态/kercall 统计
         │                         │
         │  ┌───────────────────┐  │
         │  │  OutputWriter     │  │
         │  │  ├ JsonLinesWriter│  │  JSONL 输出 (128KB 缓冲刷写)
         │  │  └ TextWriter     │  │  人类可读文本输出
         │  └───────────────────┘  │
         └────────────┬────────────┘
                      │
               ┌──────▼───────┐
               │ .jsonl / .txt │
               └──────────────┘
```

### 核心流程

1. **Prescan**（可选）— 快速扫描确定时间范围和间隙位置
2. **Parse** — 单遍回调驱动解析；间隙后仅收集 PID/TID 名称
3. **墙钟时间** — 从文件名提取结束墙钟时间，计算偏移量映射

### 环形缓冲区处理

`tracelogger -r` 环形采集模式下，停止采集后会附加进程/线程名称事件。解析器自动检测 >1s 的时间间隙来分离正常事件和采集后名称收集阶段。

### CLK 失败回退

当 `libtraceparser` 无法提供 64-bit CLK 时间时，自动使用 `RawKevTimestamps` 从原始 `time_off` 字段直接重建时间戳。

## 性能数据

| 文件大小 | 事件数 | 模式 | 耗时 |
|---------|--------|------|------|
| 159 MB | 2.9M | full | ~6s |
| 470 MB (100ms 过滤) | 23.6M | filtered | ~4.3s |
| 470 MB (全量) | 23.6M | full | ~40s |
| 1.1 GB (QNX 8.0) | 14.8M | summary | ~20s |

## 详细文档

- [kev_parser_design.md](kev_parser_design.md) — 完整架构设计、模块详解、事件字段映射
- [ring_buffer_design.md](ring_buffer_design.md) — 环形缓冲区间隙检测、两阶段处理、墙钟时间解析
