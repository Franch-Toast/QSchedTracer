# QSchedTracer

QNX 内核调度追踪器 — Ring Mode + mmap 零拷贝，输出 QST v4 格式。

## 概述

QSchedTracer 是专为 QNX Neutrino RTOS 设计的轻量级内核调度追踪工具。采用 Ring Mode 让内核将事件直接写入环形缓冲区，用户空间通过 mmap 零拷贝读取，STOP 后直接写入 QST v4 文件。

支持平台：

| 平台 | 宏定义 | Ring Mode 实现 |
|------|--------|---------------|
| QNX 7.1 (SA8650) | `QNX_710` | `ALLOCBUFFER` → mmap 物理地址 |
| QNX 8.0 (SA8797) | `QNX_800` | `LOGGER_ATTACH` + per-CPU buffer（失败时回退到 legacy paddr+mmap） |

核心特性：

- **Ring Mode + mmap 零拷贝**：内核 ring buffer 直接映射到用户空间，STOP 后零拷贝写入文件
- **QST v4 文件格式**：`QstFileHeader` (64B) → `DATA` section → `PINF` section
- **事件过滤**：默认仅调度相关事件 (Thread/Comm/KerCall)，`-a` 全量采集
- **CV 等待机制**：使用 condition_variable 等待信号，响应延迟 ~0ms，空闲 CPU 开销为零
- **信号驱动控制**：SIGINT = dump+退出，SIGUSR1 = dump+restart
- **无配置文件**：CLI 参数直接控制

## 快速开始

```bash
# 1. 编译 (QNX 7.1)
cmake -B build -DCMAKE_TOOLCHAIN_FILE=cmake/qnx710-aarch64.cmake
cmake --build build -j$(nproc)

# 2. 部署到 QNX 设备
scp build/qst_tracer root@<QNX_IP>:/tmp/

# 3. 运行采集 (Ctrl+C 停止并落盘)
/tmp/qst_tracer

# 4. 指定缓冲区和输出目录
/tmp/qst_tracer -b 20 -o /data/traces

# 5. 全量采集模式
/tmp/qst_tracer -a

# 6. 多轮采集 (另一终端发 SIGUSR1)
kill -USR1 $(pidof qst_tracer)   # 落盘+重启，可多次执行
kill -INT $(pidof qst_tracer)    # 最终停止

# 7. 解析输出文件
qst_parser tracer.*.qst -o trace.json
```

## 命令行参数

运行 `qst_tracer -h` 可查看完整帮助。

| 选项 | 说明 |
|------|------|
| `-b <MB>` | 内核 trace buffer 大小（默认 10 MB，范围 1-1024） |
| `-o <dir>` | 输出目录（默认当前目录，自动创建） |
| `-p <name>` | 输出文件前缀（默认 `tracer`） |
| `-a` | 全量采集所有事件类（默认仅调度相关） |
| `-v` | 详细输出（含 buffer 状态 dump） |
| `-h` | 显示帮助 |

### 信号控制

| 信号 | 行为 |
|------|------|
| `SIGINT` / `SIGTERM` | 停止采集 → 落盘 → 退出 |
| `SIGUSR1` | 落盘当前数据 → 重启采集（新文件） |

### 输出文件

文件名格式：`<prefix>.YYYYMMDD.HHMMSS.uuuuuu.qst`

每次 SIGUSR1 生成一个文件，最终 SIGINT 再生成一个。

## 架构

```
┌──────────────────────────────────────────────────────────────┐
│                         QNX 内核                              │
│                                                              │
│  调度器/IPC/KerCall 事件 → traceevent_t (16B)                │
│     │                                                        │
│     ▼                                                        │
│  Kernel Ring Buffer (tracebuf_t × N, Ring Mode)              │
│     │                                                        │
│     │ mmap 零拷贝 (7.1: paddr / 8.0: per-CPU vaddr)         │
└─────┼────────────────────────────────────────────────────────┘
      │
      ▼
┌──────────────────────────────────────────────────────────────┐
│                    QSchedTracer 进程                           │
│                                                              │
│  TracerEngine                                                │
│  ├── initKernelTraceBase()      IO 权限 + 清理残留            │
│  ├── setupSelfManagedMode()     ALLOCBUFFER + mmap + SETRING │
│  ├── configureEventClasses()    事件过滤配置                  │
│  ├── runLoop()                  START → CV 等待 → 信号分发    │
│  │    ├── SIGUSR1 →             dumpSelfManaged(restart=true) │
│  │    └── SIGINT  →             dumpSelfManaged(restart=false)│
│  └── dumpSelfManaged()          STOP → FLUSH → scan → write  │
│       ├── [1] scanValidRange()  扫描有效 tracebuf_t 弧段     │
│       ├── [2] openQstFile()     写 QstFileHeader (64B)       │
│       ├── [3] writeSectionHeader(DATA) + writeTracebufs       │
│       ├── [4] START → usleep(2ms) → STOP (采集进程信息)      │
│       ├── [5] writeSectionHeader(PINF) + writeTracebufs       │
│       └── [6] closeQstFile()                                 │
│                                                              │
│  DataBuffer   元数据容器 (clock_freq, wallclock)              │
│  DataManager  QST v4 文件 I/O                                │
└──────────────────────────────────────────────────────────────┘
      │
      ▼
┌──────────────────────────────────────────────────────────────┐
│  输出: prefix.YYYYMMDD.HHMMSS.uuuuuu.qst                    │
│                                                              │
│  QST v4 布局:                                                │
│    QstFileHeader (64B): magic, clock_freq, start/end_ns, ... │
│    DATA SectionHeader (24B) + raw tracebuf_t[] (事件数据)    │
│    PINF SectionHeader (24B) + raw tracebuf_t[] (进程/线程名) │
└──────────────────────────────────────────────────────────────┘
```

### 采集流程

1. **初始化**：获取 IO 权限 → kill 残留 tracelogger → ALLOCBUFFER → mmap → SETRINGMODE
2. **事件配置**：根据 `-a` 选择 Scheduling/All 模式配置内核事件过滤
3. **采集**：`STARTNOSTATE` → 内核持续写入 ring buffer → 主线程 CV 等待信号
4. **落盘**：STOP → FLUSH → scanValidRange → 写 DATA section → START(procinfo 2ms) → STOP → 写 PINF section
5. **重启**（SIGUSR1）：SETRINGMODE → 重新配置事件 → STARTNOSTATE
6. **清理**：STOP → munmap → DEALLOCBUFFER → LOGGER_DETACH (8.0)

## 编译

### 前置依赖

| 依赖 | 版本 | 说明 |
|------|------|------|
| QNX SDP | 7.1 / 8.0 | 交叉编译工具链 |
| CMake | >= 3.14 | 构建系统 |
| C++ | C++17 | 语言标准 |
| spdlog | 1.15+ | 日志库（已内置于 `spdlog/`） |

### CMake 交叉编译（推荐）

```bash
# QNX 7.1 aarch64
export QNX_HOST=/path/to/toolchains_qnx/host/linux/x86_64
export QNX_TARGET=/path/to/toolchains_qnx/target/qnx7
cmake -B build_71 \
    -DCMAKE_TOOLCHAIN_FILE=cmake/qnx710-aarch64.cmake \
    -DCMAKE_BUILD_TYPE=Release
cmake --build build_71 -j$(nproc)

# QNX 8.0 aarch64
export QNX_HOST=/path/to/toolchains_qnx_sdp8/host/linux/x86_64
export QNX_TARGET=/path/to/toolchains_qnx_sdp8/target/qnx
cmake -B build_80 \
    -DCMAKE_TOOLCHAIN_FILE=cmake/qnx800-aarch64.cmake \
    -DQNX80=ON \
    -DCMAKE_BUILD_TYPE=Release
cmake --build build_80 -j$(nproc)
```

### build.bash 快捷脚本

```bash
./build.bash              # QNX 7.1 Release
./build.bash qnx80        # QNX 8.0 Release
./build.bash debug         # QNX 7.1 Debug
./build.bash qnx80 debug  # QNX 8.0 Debug
./build.bash clean         # 清理
```

### Linux Host 编译（开发调试用）

在 Linux 上编译时，QNX 特有的内核 trace API 代码被 `#ifdef __QNX__` 跳过，仅用于验证代码结构和非 QNX 分支逻辑。

```bash
cmake -B build_linux
cmake --build build_linux -j$(nproc)
```

### 平台宏说明

| CMake 选项 | 生成的宏 | 含义 |
|-----------|---------|------|
| 默认 | `-DQNX_710` | QNX 7.1 模式 |
| `-DQNX80=ON` | `-DQNX_800` | QNX 8.0 模式 |

## 项目结构

```
QSchedTracer/
├── CMakeLists.txt                      # 构建配置
├── build.bash                          # 编译快捷脚本
├── README.md
│
├── include/qst/                        # 头文件
│   ├── types.hpp                       # QST v4 文件格式定义
│   ├── log.hpp                         # spdlog 日志封装
│   ├── config/
│   │   └── types.hpp                   # TracerConfig, EventFilter
│   ├── core/
│   │   ├── tracer_engine.hpp           # 追踪引擎 (Ring Mode + CV)
│   │   └── data_buffer.hpp             # 元数据容器
│   └── data/
│       └── data_manager.hpp            # QST v4 文件写入器
│
├── src/                                # 源文件 (~1,200 行)
│   ├── main.cpp                        # CLI 入口 + 信号处理
│   ├── core/
│   │   ├── tracer_engine.cpp           # 引擎生命周期 (构造/运行/CV等待/清理)
│   │   ├── tracer_event_config.cpp     # QNX 内核事件类过滤配置
│   │   └── tracer_ring_dump.cpp        # Ring Buffer 分配/扫描/落盘
│   └── data/
│       └── data_manager.cpp            # QST v4 文件 I/O
│
├── cmake/                              # 交叉编译工具链文件
│   ├── qnx710-aarch64.cmake
│   └── qnx800-aarch64.cmake
│
├── spdlog/                             # spdlog 日志库 (header-only, vendored)
├── tests/                              # TraceEvent API 诊断工具
│
├── kev_parser/                         # .kev 文件解析器 (C++, 独立子项目)
│   ├── README.md                       # 含编译和使用方法
│   └── ...
│
└── qst_parser/                         # .qst 文件解析器 (C++, 独立子项目)
    ├── README.md                       # 含编译和使用方法
    └── ...
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
0x10    4B      entry_count (tracebuf_t 块数)
0x14    4B      reserved
--- (24 bytes total) ---
```

### 事件过滤

**默认 (Scheduling)**：

| 类别 | 模式 | 说明 |
|------|------|------|
| Thread | Wide | 全部线程状态事件（含 64-bit 扩展参数） |
| VThread | Wide | 虚拟线程（QNX 7.1 only） |
| Process | Default | 进程创建/销毁 |
| Comm | Wide | IPC 通信事件 |
| Control | Event | CONTROLBUFFER only |
| KerCall | Wide, 选择性 | MUTEX/SEM/CONDVAR/MSG/SCHED_YIELD |

**全量 (-a)**：`ADDALLCLASSES + SETCLASSWIDE`，包括 System、Interrupt 和全部 KerCall。

### 平台差异

| 特性 | QNX 7.1 | QNX 8.0 |
|------|---------|---------|
| Buffer 分配 | `ALLOCBUFFER` → paddr_t | `LOGGER_ATTACH` → per-CPU vaddr |
| 内存映射 | `mmap(MAP_PHYS, paddr)` | ALLOCBUFFER 直接返回虚拟地址 |
| 8.0 回退 | - | ATTACH 失败 → legacy paddr+mmap |
| VThread | 支持 | 不支持 |
| Buffer 计算 | total = MB * 1024 / 16 | per_cpu = total / num_cpus |
| os_version | 710 | 800 |

## 相关工具

| 工具 | 路径 | 说明 |
|------|------|------|
| **kev_parser** | `kev_parser/` | .kev 文件 → JSONL/text 转换器，支持 QNX 7.1 和 8.0 |
| **qst_parser** | `qst_parser/` | .qst 文件 → JSON/Chrome Trace 转换器，C++ 实现 |

两个解析器均为独立 CMake 子项目，各自有完整的 README 和 `--help` 文档。

## License

MIT License
