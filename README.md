# QSchedTracer

**QNX 调度追踪器** — 轻量级飞行记录仪，Ring Mode + mmap 零拷贝，输出 QST v4 格式 (v3.0)

## 概述

QSchedTracer 是专为 QNX Neutrino RTOS 设计的内核调度追踪工具。采用 Ring Mode + mmap 零拷贝架构，持续记录调度事件，通过信号控制落盘和重启。

支持平台：

| 平台 | 宏定义 | Ring Mode 实现 |
|------|--------|---------------|
| QNX 7.1 (SA8650) | `LP8650` | `ALLOCBUFFER` → mmap 物理地址 |
| QNX 8.0 (SA8797) | `LP8797` | `LOGGER_ATTACH` + per-CPU buffer |

核心特性：

- **Ring Mode + mmap 零拷贝**：内核 ring buffer 直接映射到用户空间，STOP 后零拷贝写入文件
- **QST v4 文件格式**：`QstFileHeader` (64B) → `DATA` section (raw tracebuf_t) → `PINF` section (进程信息)
- **事件过滤**：默认仅调度相关事件 (Thread/Comm/KerCall)，`-a` 全量采集
- **信号驱动控制**：SIGINT = dump+退出，SIGUSR1 = dump+restart（支持多轮采集）
- **无配置文件**：CLI 参数直接控制，无需 JSON 配置

## 快速开始

```bash
# 1. 编译
./build.bash

# 2. 部署到 QNX 设备
scp build/qnx_aarch64/qst_tracer root@<QNX_IP>:/tmp/

# 3. 运行采集 (Ctrl+C 停止并落盘)
ssh root@<QNX_IP> '/tmp/qst_tracer'

# 4. 指定缓冲区大小和输出目录
ssh root@<QNX_IP> '/tmp/qst_tracer -b 20 -o /data/traces'

# 5. 全量采集模式
ssh root@<QNX_IP> '/tmp/qst_tracer -a'

# 6. 多轮采集 (另一个终端发送 SIGUSR1 触发 dump+restart)
ssh root@<QNX_IP> 'kill -USR1 $(pidof qst_tracer)'  # 可多次执行

# 7. 取回数据并解析
scp root@<QNX_IP>:/tmp/tracer.*.qst .
python3 -m qst_parser tracer.*.qst -o trace.json
```

## 架构

```
┌─────────────────────────────────────────────────────────────┐
│                        QNX 内核                              │
│                                                             │
│  调度器/IPC/KerCall 事件 → traceevent_t (16B)               │
│     │                                                       │
│     ▼                                                       │
│  Kernel Ring Buffer (tracebuf_t × N, Ring Mode)             │
│     │                                                       │
│     │ mmap 零拷贝 (QNX 7.1: 物理地址 / QNX 8.0: per-CPU)   │
└─────┼───────────────────────────────────────────────────────┘
      │
      ▼
┌─────────────────────────────────────────────────────────────┐
│                   QSchedTracer 进程                          │
│                                                             │
│  TracerEngine                                               │
│  ├── initKernelTraceBase()     IO 权限 + 清理残留            │
│  ├── setupSelfManagedMode()    ALLOCBUFFER + mmap + SETRING │
│  ├── configureEventClasses()   事件过滤配置                  │
│  ├── runLoop()                 START → poll → signal dispatch │
│  │    ├── SIGUSR1 →            dumpSelfManaged(restart=true) │
│  │    └── SIGINT  →            dumpSelfManaged(restart=false)│
│  └── dumpSelfManaged()         STOP → FLUSH → scan → write  │
│       ├── [1] scanValidRange() 扫描有效 tracebuf_t 区间     │
│       ├── [2] DataManager.openQstFile()    写 QstFileHeader │
│       ├── [3] writeContiguousTracebufs()   写 DATA section  │
│       ├── [4] START → usleep → STOP (采集进程信息)          │
│       ├── [5] writeContiguousTracebufs()   写 PINF section  │
│       └── [6] closeQstFile()                                │
│                                                             │
│  DataBuffer (元数据容器: clock_freq, wallclock)              │
│  DataManager (QST v4 文件 I/O)                              │
└─────────────────────────────────────────────────────────────┘
      │
      ▼
┌─────────────────────────────────────────────────────────────┐
│  输出: prefix.YYYYMMDD.HHMMSS.uuuuuu.qst                   │
│                                                             │
│  QST v4 文件布局:                                           │
│    QstFileHeader (64B)                                      │
│    ├── magic: 'QST4' (0x51535434)                           │
│    ├── version: 4                                           │
│    ├── clock_freq, capture_start/end_ns, num_cpus, ...      │
│    SectionHeader (24B, DATA)                                │
│    ├── raw tracebuf_t[] (调度事件数据)                       │
│    SectionHeader (24B, PINF)                                │
│    └── raw tracebuf_t[] (进程/线程名称信息)                  │
└─────────────────────────────────────────────────────────────┘
```

## 命令行接口

```
QSchedTracer - QNX Ring Mode Scheduler Tracer (QST v4)

Usage: qst_tracer [options]

Options:
  -b <MB>    Buffer size in MB (default: 10)
  -o <dir>   Output directory (default: .)
  -p <name>  Output file prefix (default: tracer)
  -a         Capture all events (default: scheduling only)
  -v         Verbose output
  -h         Show this help

Output file:
  prefix.YYYYMMDD.HHMMSS.uuuuuu.qst

Signals:
  SIGINT/SIGTERM  Dump and exit
  SIGUSR1         Dump and restart (for multi-round testing)
```

示例：

```bash
qst_tracer                     # 默认：10MB 缓冲，调度事件，输出到当前目录
qst_tracer -b 20               # 20MB 缓冲区
qst_tracer -o /data/traces     # 指定输出目录
qst_tracer -p myapp            # 输出文件前缀为 myapp
qst_tracer -a                  # 全量采集所有事件类别
qst_tracer -a -b 50 -v         # 全量采集 + 50MB 缓冲 + 详细日志
```

### 多轮采集 (dump+restart)

```bash
# 终端 1: 启动采集（verbose 模式可观察 SETRINGMODE restart 行为）
/tmp/qst_tracer -b 50 -o /tmp -p test -v

# 终端 2: 触发 dump+restart (可多次执行)
kill -USR1 $(pidof qst_tracer)
kill -USR1 $(pidof qst_tracer)   # 第二轮

# 终端 2: 最终停止
kill -INT $(pidof qst_tracer)

# 每次 SIGUSR1 生成一个 .qst 文件，最终 SIGINT 再生成一个
ls /tmp/test.*.qst
```

## 编译

### 环境要求

| 依赖 | 版本 | 说明 |
|------|------|------|
| QNX SDP | 7.1+ | 交叉编译工具链 |
| C++ | C++17 | 语言标准 |
| spdlog | 1.15+ | 日志库 (included in `spdlog/`) |

### 方法一：build.bash

```bash
# QNX 7.1 Release (默认)
./build.bash

# QNX 8.0 Release
./build.bash qnx80

# Debug 版本
./build.bash debug
./build.bash qnx80 debug

# 清理
./build.bash clean
```

输出：`build/qnx71_aarch64/qst_tracer` 或 `build/qnx80_aarch64/qst_tracer`

### 方法二：CMake

```bash
# QNX 7.1 交叉编译
export QNX_HOST=/path/to/toolchains_qnx/host/linux/x86_64
export QNX_TARGET=/path/to/toolchains_qnx/target/qnx7
cmake -B build_qnx71 -DCMAKE_TOOLCHAIN_FILE=cmake/qnx710-aarch64.cmake -DCMAKE_BUILD_TYPE=Release
cmake --build build_qnx71

# QNX 8.0 交叉编译
export QNX_HOST=/path/to/toolchains_qnx_sdp8/host/linux/x86_64
export QNX_TARGET=/path/to/toolchains_qnx_sdp8/target/qnx
cmake -B build_qnx80 -DCMAKE_TOOLCHAIN_FILE=cmake/qnx800-aarch64.cmake -DQNX80=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build_qnx80
```

### 方法三：Bazel (Deeproute CI)

```bash
# QNX 7.1 (SA8650)
bazel build --config=qnx_lp8650 //:qst_tracer_bin

# QNX 8.0 (SA8797)
bazel build --config=qnx_lp8797 //:qst_tracer_bin

# 构建发布包
bazel build --config=qnx_lp8650 //:qst_tracer_release_package
```

## 项目结构

```
QSchedTracer/
├── build.bash                      # 编译脚本 (QNX 7.1 / 8.0)
├── CMakeLists.txt                  # CMake 配置
├── BUILD                           # Bazel 配置
├── README.md
│
├── include/qst/
│   ├── types.hpp                   # QST v4 文件格式结构体
│   ├── log.hpp                     # 日志工具 (spdlog)
│   ├── config/
│   │   └── types.hpp               # TracerConfig, EventFilter
│   ├── core/
│   │   ├── data_buffer.hpp         # 元数据容器 (clock_freq, wallclock, header-only)
│   │   └── tracer_engine.hpp       # 追踪引擎 (Ring Mode + mmap)
│   └── data/
│       └── data_manager.hpp        # QST v4 文件写入器
│
├── src/
│   ├── main.cpp                    # CLI 入口
│   ├── core/
│   │   ├── tracer_engine.cpp       # 核心生命周期 (构造/运行/清理)
│   │   ├── tracer_event_config.cpp # 事件类配置 (QNX only)
│   │   └── tracer_ring_dump.cpp    # Ring Buffer 管理 + 数据转储
│   └── data/
│       └── data_manager.cpp
│
├── cmake/
│   ├── qnx710-aarch64.cmake       # QNX 7.1 交叉编译工具链
│   └── qnx800-aarch64.cmake       # QNX 8.0 交叉编译工具链
│
├── spdlog/                         # spdlog (header-only, vendored)
│
├── kev_parser/                     # .kev 文件解析器 (独立子项目)
│   ├── CMakeLists.txt
│   ├── README.md
│   ├── kev_parser_design.md
│   ├── ring_buffer_design.md
│   ├── include/kev/                # 解析器头文件
│   └── src/                        # 解析器源码
│
└── qst_parser/                     # .qst 文件解析器 (Python)
    ├── __main__.py
    ├── core/                       # 解析核心
    ├── exporters/                  # JSONL / Perfetto 导出
    └── models/                     # 数据模型
```

## 技术细节

### QST v4 文件格式

```
偏移    大小    字段
0x00    4B      magic:    0x51535434 ('QST4')
0x04    4B      version:  4
0x08    4B      header_size: 64
0x0C    4B      flags:    0 (little-endian)
0x10    8B      clock_freq (cycles_per_sec)
0x18    8B      capture_start_ns (epoch nanoseconds)
0x20    8B      capture_end_ns
0x28    4B      num_cpus
0x2C    4B      os_version (710 / 800)
0x30    4B      tracebuf_size
0x34    4B      data_offset
0x38    8B      reserved (bufs_per_cpu for QNX 8.0)
--- (64 bytes total) ---

SectionHeader (DATA / PINF):
0x00    4B      magic:    'DATA' (0x44415441) or 'PINF' (0x50494E46)
0x04    4B      version:  1
0x08    8B      payload_size (bytes)
0x10    4B      entry_count (number of tracebuf_t blocks)
0x14    4B      reserved
--- (24 bytes total) ---
```

### 事件过滤 (Scheduling 模式)

默认 `-a` 未指定时，仅采集调度相关事件：

| 类别 | 模式 | 说明 |
|------|------|------|
| Thread | Wide | 全部线程状态事件 |
| VThread | Wide | 虚拟线程 (QNX 7.1 only) |
| Process | Default | 进程创建/销毁 |
| Comm | Wide | IPC 通信事件 |
| Control | Event | CONTROLBUFFER only |
| KerCall | Wide, 选择性 | MUTEX/SEM/CONDVAR/MSG/SCHED_YIELD |

`-a` 模式下采集所有事件类别 (`ADDALLCLASSES + SETCLASSWIDE`)。

### 平台差异

| 特性 | QNX 7.1 (LP8650) | QNX 8.0 (LP8797) |
|------|-------------------|-------------------|
| Buffer 分配 | `ALLOCBUFFER` → paddr_t | `LOGGER_ATTACH` → per-CPU |
| 内存映射 | `mmap(MAP_PHYS, paddr)` | `ALLOCBUFFER` 直接返回虚拟地址 |
| VThread | 支持 | 不支持 |
| Buffer 计算 | total = size_mb * 1024 / 16 | per_cpu = total / num_cpus |

## 相关工具

| 工具 | 路径 | 说明 |
|------|------|------|
| **kev_parser** | `kev_parser/` | .kev 文件解析器 (C++), 将 tracelogger kev 文件转为 JSONL/text |
| **qst_parser** | `qst_parser/` | .qst 文件解析器 (Python), 将 QST 文件转为 JSON/Perfetto 格式 |

## License

MIT License
