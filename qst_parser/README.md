# QST Parser

QNX QST v4 trace 文件的高性能流式解析器，输出 Chrome JSON Trace Event Format，可直接在 [Perfetto UI](https://ui.perfetto.dev) 中打开进行可视化分析。

支持 QNX 7.1 和 QNX 8.0 两个版本的 trace 数据。

## 项目结构

```
qst_parser/
├── CMakeLists.txt                  # 构建配置
├── include/
│   └── qst/
│       ├── constants.h             # 事件类枚举、线程状态名、格式常量
│       ├── timestamp.h             # 32位周期计数器时间戳工具
│       ├── file_access.h           # mmap 零拷贝文件访问
│       ├── buffer_reader.h         # QST v4 结构读取 (FileHeader, DATA, PINF)
│       ├── event_names.h           # 事件名称表接口声明
│       ├── json_writer.h           # Chrome JSON Trace Event Format 流式写入
│       └── streaming_parser.h      # 核心解析引擎类声明
├── src/
│   ├── main.cpp                    # CLI 入口、参数解析、usage 帮助
│   ├── streaming_parser.cpp        # 核心管线：时间戳重建 + 事件分发
│   ├── handler_thread.cpp          # Thread/Process 事件处理 + flush
│   ├── handler_events.cpp          # Interrupt/KerCall/Comm/System 事件处理
│   ├── flow_tracker.cpp            # Sync/IPC/Signal flow 追踪
│   ├── kercall_tables.cpp          # KerCall 名称表 + 字段数组 (数据驱动)
│   └── event_names.cpp             # Comm/System 名称表 + 参数格式化
└── README.md
```

### 架构设计

解析引擎采用两阶段流水线：

1. **反向时间戳重建** (Backward Carry Chain)
   - QNX trace 事件只记录 32 位 ClockCycles() 低位（150MHz 下约 28.6 秒回绕一次）
   - 从文件尾部锚点开始，逆序计算每个 buffer 的边界时间戳
   - QNX 8.0 支持 per-CPU buffer 分组，每组独立建链

2. **正向事件分发** (Forward Dispatch)
   - 按时间顺序遍历每个 buffer 中的 16 字节事件
   - 根据事件类（thread/interrupt/kercall/comm/system）分发到对应处理器
   - Wide 模式事件（COMBINE_BEGIN/CONT/END）自动合并
   - 流式写入 JSON，内存占用恒定

### 支持的事件类型

| 事件类 | 输出类型 | 说明 |
|--------|----------|------|
| Thread/VThread | Duration Slice (X) | 线程状态变迁，含 priority/policy/vthread |
| CPU Running | Duration Slice (X) | 每个 CPU 上当前运行的线程 |
| Interrupt | Duration Slice (X) | IRQ 进入/退出配对 |
| KerCall | Instant (i) | 内核调用 Enter/Exit/Int，含参数解析 |
| Comm | Instant (i) | IPC 通信事件 (Send/Reply/Pulse/Signal) |
| System | Instant (i) | 系统事件 (MMAP/MUNMAP/Timer/IPI 等) |
| Process | Instant (i) | 进程创建/销毁 |

### Flow 事件追踪

| Flow 类型 | 触发条件 | 说明 |
|-----------|----------|------|
| sync_mutex/condvar/sem | MutexLock→READY | 同步原语的 wait→wake 链路 |
| ipc_msg | MsgSend→MsgReceive | 消息发送到接收 |
| ipc_send_ack | MsgReceive→sender | 接收确认返回发送方 |
| ipc_reply | MsgReply→sender READY | 回复到发送方唤醒 |
| signal | SignalKill→SIGWAITINFO/SIGSUSPEND READY | 信号投递到目标唤醒 |

## 编译

### 依赖

- CMake >= 3.14
- C++17 编译器 (GCC 7+ / Clang 5+)
- Linux 系统（使用 mmap）

### 构建步骤

```bash
cd qst_parser
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

生成可执行文件 `build/qst_parser`。

## 使用方法

### 基本用法

```bash
# 解析并导出为 Chrome JSON（默认输出 <input>.json）
./build/qst_parser trace.qst

# 指定输出路径
./build/qst_parser trace.qst -o output.json

# 仅显示文件摘要，不导出
./build/qst_parser trace.qst -n
```

### 命令行参数

| 选项 | 说明 |
|------|------|
| `-o, --output PATH` | 输出文件路径（默认: `<input>.json`） |
| `-f, --format FMT` | 输出格式，当前仅支持 `json` |
| `-n, --no-export` | 仅解析并打印文件摘要，不生成输出文件 |
| `-l, --lightweight` | 轻量模式：仅解析 Thread/CPU 事件，跳过 KerCall/Comm/System |
| `-v, --verbose` | 详细输出 |
| `--pid PID` | 按进程 ID 过滤（可重复指定多个） |
| `--tid TID` | 按线程 ID 过滤（需配合 `--pid`，可重复） |
| `-h, --help` | 显示完整帮助信息（含所有选项说明、输出格式和使用示例） |

### 使用示例

```bash
# 查看文件基本信息
./build/qst_parser trace.qst -n

# 解析并导出 JSON（默认输出 trace.json）
./build/qst_parser trace.qst

# 指定输出路径
./build/qst_parser trace.qst -o /tmp/output.json

# 轻量模式（仅 Thread/CPU 事件，跳过 KerCall/Comm/System）
./build/qst_parser trace.qst -l -o thread_only.json

# 过滤特定进程
./build/qst_parser trace.qst --pid 12345 --pid 67890

# 过滤特定线程
./build/qst_parser trace.qst --pid 12345 --tid 1 --tid 2
```

### 查看结果

在浏览器中打开 https://ui.perfetto.dev ，将生成的 `.json` 文件拖入即可查看：

- **CPU Tracks**: 每个 CPU 的调度时间线
- **Process/Thread Tracks**: 每个线程的状态变迁
- **IRQ Tracks**: 中断处理时间
- **KerCall/Comm/System**: 即时事件标记
- **Flow Arrows**: IPC 消息传递、同步原语唤醒链路

## QST v4 文件格式

```
┌─────────────────────────────┐
│  QstFileHeader (64 bytes)   │  magic='QST4', version=4
│  clock_freq, num_cpus, ...  │  os_version: 710 or 800
├─────────────────────────────┤
│  DATA Section Header (24B)  │  magic='DATA'
├─────────────────────────────┤
│  tracebuf_t[0]              │  每个 tracebuf_t 包含:
│    header (184B)            │    - flags, num_events, seq
│    events[0..N] (16B each)  │    - 16字节事件数组
│  tracebuf_t[1]              │
│  ...                        │
├─────────────────────────────┤
│  PINF Section Header (24B)  │  magic='PINF'
├─────────────────────────────┤
│  tracebuf_t blocks          │  进程/线程名称信息
│  (process/thread names)     │
└─────────────────────────────┘
```

每个 16 字节事件的结构：

```
bits [9:0]   = int_event   (事件子类型)
bits [14:10] = int_class   (事件大类: thread/int/kercall/comm/system)
bits [29:24] = cpu_id
bits [31:30] = struct_type (simple / combine_begin / cont / end)
d0 (4B)      = 32-bit cycle timestamp
d1 (4B)      = data word 1
d2 (4B)      = data word 2
```

## 性能

在 140MB QST 文件 (QNX 7.1, 8.6M 事件) 上的测试结果：

| 指标 | 值 |
|------|-----|
| 解析+导出时间 | ~22 秒 |
| 输出 JSON 大小 | ~1 GB |
| 事件处理速率 | ~390K events/sec |
| 内存占用 | < 200 MB (mmap + streaming) |

## 许可

内部工具，仅供项目使用。
