# QST Parser v4.4.0

QNX Trace Event 流式解析器，解析 QSchedTracer 生成的 `.qst` v4 文件，支持导出为 **Perfetto protobuf** 或 **Chrome JSON Trace Event Format**。

> **v4.4.0**: 新增事件过滤功能——按时间范围、PID、TID 过滤输出：
> - `--time-start` / `--time-end`: 绝对时间范围过滤（格式 `YYYY-MM-DD HH:MM:SS.nnnnnnnnn`）
> - `--pid`: 按进程 ID 过滤（Thread/KerCall/Comm/Flow 事件；Interrupt/System 保留）
> - `--tid`: 在 PID 内按线程 ID 过滤（需配合 `--pid` 使用）
> - 过滤仅影响输出，内部状态跟踪（flow 匹配等）不受影响
>
> **v4.3.0**: 新增 Flow Events——在 Perfetto 中以箭头可视化线程间因果关系：
> - **Sync flow**: mutex unlock → waiter wakeup, condvar signal → waiter wakeup, sem post → waiter wakeup
> - **IPC flow**: MsgSend → MsgReceive (消息投递), MsgReceive → 发送者确认 (ack), MsgReply → 客户端唤醒 (reply)
> - **Signal flow**: SignalKill → SIGWAITINFO/SIGSUSPEND wakeup（支持 QNX pid=0 自身信号约定）
>
> **v4.2.0**: 修复 KerCall ENTER/EXIT/INT 分类、64 位事件字段布局、COMM 字段碰撞。pid/tid 自动名称解析。

## 架构设计

### 两级流水线

```
Stage 1: Metadata + PINF
  ├── 解析 QstFileHeader (64B)
  ├── 扫描 DATA section buffer 元数据 (seq, num_events, file_offset)
  └── 解析 PINF section 提取进程/线程名称

Stage 2: Backward Timestamp + Forward Dispatch
  ├── 反向遍历计算每个 buffer 的基准时间戳 (处理 32-bit cycle 回绕)
  └── 正向遍历分发事件，即时解析并写入输出:
      ├── Thread State → Duration Slice (线程 Track) + Flow 检测
      ├── CPU Running → Duration Slice (CPU Track)
      ├── Interrupt → Duration Slice / Instant (CPU IRQ Track)
      ├── KerCall → Instant (线程 Track) + Sync/IPC Flow 注册
      ├── Comm → Instant (线程 Track) + IPC Reply 注册
      ├── System → Instant (CPU Track)
      ├── Flow Events → 箭头连接因果事件 (sync/ipc/signal)
      └── Output Filter → 按时间/PID/TID 过滤写入 (内部状态始终维护)
```

所有事件在 forward dispatch 阶段即时解析和写入，无 deferred storage。KerCall/Comm 事件通过 `_cpu_pending` 映射确定运行中的线程。

### 文件访问

通过 `FileAccess` 抽象层访问 QST 文件：
- `MmapAccess` — 零拷贝 mmap 映射（默认）
- `SeekReadAccess` — seek+read 回退方案

### 输出格式

| 格式 | 扩展名 | Writer | 特点 |
|------|--------|--------|------|
| Perfetto protobuf | `.perfetto` | `BatchingWriter` | 原生 Perfetto 格式，最佳兼容 |
| Chrome JSON | `.json` | `JsonTraceWriter` | 纯文本流式写入，无 protobuf 依赖，适合大文件 |

### Wide Mode Combine 事件重组

QNX trace 事件有两种模式：
- **Fast mode** (SIMPLE)：每个事件 1 个 `traceevent_t` 槽位，包含 2 个数据字段
- **Wide mode** (COMBINE)：每个事件跨多个槽位 (CB→CC→CE)，包含完整数据字段

解析器对所有事件类 (Thread, KerCall, COMM, System) 均支持 combine 事件重组：
- `CB` (begin): 缓存首批数据
- `CC` (cont): 追加中间数据
- `CE` (end): 追加尾部数据，合并后发射单一事件

合并后的数据字段根据 SAT 文档中的字段布局表映射为语义化名称（如 `coid`, `sparts`, `sync_p` 等），而非原始 `d0/d1/d2`。

### pid/tid 名称自动解析

事件参数中的 `pid`/`tid` 相关字段自动映射为人可读名称：

| 字段类型 | 原始值 | 解析后显示 |
|----------|--------|-----------|
| pid 类 (`info_pid`, `target_pid`, `pid` 等) | `2207843` | `opt/usr/bin/lpLocation(2207843)` |
| tid 类 (`info_tid`, `target_tid`, `tid` 等) | `4` | `qgptp_OSCallback:4` |

解析逻辑：
- pid 字段查找 `process_names` 表（来自 PINF + DATA）
- tid 字段先定位对应的 pid 值（按命名惯例：`info_tid` → `info_pid`），再查找 `thread_names` 表
- 查找失败时保留原始数字
- 事件 owner 的 `pid`/`tid`（已有 `process`/`thread` 标签的）不重复解析

### Perfetto Track 层次

```
CPU Tracks (统一分组)
├── CPU 0                   ← Running Thread Duration Slices
├── CPU 0 IRQ               ← Interrupt Slices + System Instants
├── CPU 1
├── CPU 1 IRQ
└── ...

Process Name (pid)
├── Thread Name (tid)       ← Thread State Duration Slices
│                             + KerCall Instants + Comm Instants
│                             + Flow Event Endpoints (sync/ipc arrows)
├── Thread Name (tid)
└── ...
```

### QNX 版本差异

| | QNX 7.1 | QNX 8.0 |
|---|---|---|
| DATA buffer 布局 | 全局共享 ring buffer | Per-CPU 独立 ring buffer |
| 排序方式 | 全局按 `seq_buff_num` 排序 | 按 CPU 分组，组内按 `seq` 排序 |
| 时间戳锚定 | 全局最后 event cycle ↔ `capture_end_ns` | 找全局最大 cycle，各 CPU 独立锚定 |
| `bufs_per_cpu` | 0 (不使用) | >0, 用于分组 |
| Kercall 编号 | QNX 7.1 `kercalls.h` (92 entries) | QNX 8.0 `kercalls.h` (93 entries) |

## 项目结构

```
qst_parser/
├── __init__.py              # 包入口, 导出 StreamingParser
├── __main__.py              # python -m qst_parser
├── main.py                  # CLI 入口
├── constants.py             # 公共常量 (文件格式、事件类、中断/结构类型)
├── event_names.py           # 事件名称映射表 + 字段布局表 (含 64-bit 专用表)
│
├── core/                    # 解析核心
│   ├── streaming_parser.py  # 两级流水线主逻辑 + combine 重组 + flow tracking
│   ├── buffer_reader.py     # Stage 1: 文件结构读取
│   ├── file_access.py       # mmap / seek+read 文件访问抽象
│   └── timestamp.py         # 32-bit cycle 回绕处理
│
├── models/
│   └── headers.py           # QstFileHeader, SectionHeader, KBufEntry
│
├── exporters/
│   ├── json_writer.py       # Chrome JSON Trace Event Format writer (incl. flow events)
│   └── perfetto/
│       ├── utils.py         # BatchingWriter, write_slice, write_instant
│       └── tracks.py        # TrackManager (CPU / Process / Thread tracks)
│
├── classes/
│   └── thread/
│       └── constants.py     # THREAD_STATES, MAX_TH_STATE_NUM
│
└── docs/
    └── parser_design_v4.md  # 详细设计文档
```

## 使用方法

### 命令行

```bash
# 解析并导出 Perfetto protobuf（默认）
python3 -m qst_parser trace.qst

# 导出为 Chrome JSON 格式
python3 -m qst_parser trace.qst --format json

# 指定输出路径
python3 -m qst_parser trace.qst -o output.perfetto

# 仅输出摘要，不导出
python3 -m qst_parser trace.qst --no-export

# 轻量模式 (仅 thread/CPU events，跳过 kercall/comm/system)
python3 -m qst_parser trace.qst --lightweight

# 按时间范围过滤 (绝对时间，纳秒精度)
python3 -m qst_parser trace.qst --format json \
  --time-start "1970-01-06 02:37:40.000000000" \
  --time-end "1970-01-06 02:37:42.000000000"

# 按 PID 过滤 (可指定多个)
python3 -m qst_parser trace.qst --format json --pid 151589 2207849

# 按 PID + TID 过滤 (TID 需配合 PID 使用)
python3 -m qst_parser trace.qst --format json --pid 151589 --tid 1 2 3

# 组合: 时间范围 + PID + TID
python3 -m qst_parser trace.qst --format json \
  --time-start "1970-01-06 02:37:40.000000000" \
  --time-end "1970-01-06 02:37:42.000000000" \
  --pid 151589 --tid 1 2 3

# 详细输出
python3 -m qst_parser trace.qst -v
```

### Python API

```python
from qst_parser.core.streaming_parser import StreamingParser

sp = StreamingParser("trace.qst", verbose=True)

# 完整解析 + Perfetto protobuf 导出
stats = sp.parse_and_export("trace.perfetto")

# 完整解析 + JSON 导出
stats = sp.parse_and_export("trace.json", fmt="json")

# 带过滤的导出
sp = StreamingParser(
    "trace.qst",
    filter_pids=frozenset([151589, 2207849]),
    filter_tids=frozenset([1, 2, 3]),
    filter_time_start_ns=441468000000000000,
    filter_time_end_ns=441470000000000000)
stats = sp.parse_and_export("filtered.json", fmt="json")

# 仅摘要
summary = sp.parse_summary()
print(f"Total events: {summary['total_events']}")
```

## 事件类别

### Thread (PR_TH class, int_event < MAX_TH_STATE_NUM)

线程状态变化 → **Duration Slice** on Thread Track

状态包括：RUNNING, READY, SEND, RECEIVE, REPLY, MUTEX, CONDVAR, SEM,
NANOSLEEP, STOPPED, DEAD 等。

- **Fast mode**: 仅包含 `pid`, `tid`
- **Wide mode**: 包含 `pid`, `tid`, `priority`, `policy`（含 APS 时还有 `partition_id`, `sched_flags`）

### Interrupt (INT class)

中断进入/退出 → **Duration Slice** on CPU IRQ Track（配对成功时）
未配对的退出 → **Instant** on CPU IRQ Track

### KerCall (KER_CALL class)

内核调用 (MsgSendv, SyncMutexLock, SignalKill 等) → **Instant** on Thread Track

- 88 个 ENTER 字段布局 + 86 个 EXIT 字段布局（覆盖全部 kercall）
- 64 位事件独立字段表（`_KERCALL_ENTER_WIDE64` / `_KERCALL_EXIT_WIDE64`），将 `(64)` 字段拆为 `_lo`/`_hi`
- Fast mode: 2 个语义化字段（如 `coid`, `msg[0]`）
- Wide mode: 完整字段（如 `coid`, `sparts`, `rparts`, `msg[0]`, `msg[1]`, `msg[2]`）
- 支持 QNX 7.1 (92 calls) 和 QNX 8.0 (93 calls) 的不同编号
- ENTER/EXIT/INT 分类基于 `int_event` 的位域解码（bit 7: EXIT, bit 8: INT, bit 9: 64-bit）
- 数据字段中的 `pid`/`tid`（如 `SignalKill` 的目标 pid）自动重命名为 `target_pid`/`target_tid` 以避免与 owner 碰撞

### Comm (COMM class)

通信事件 (SMSG, RMSG, SPULSE, RPULSE, SIGNAL 等) → **Instant** on Thread Track

12 种 COMM 事件，每种有描述性字段名（如 `rcvid`, `target_pid`, `si_signo`）。
COMM 字段中的 `pid`/`tid` 统一使用 `target_pid`/`target_tid` 前缀，与 owner 字段区分。
SIGNAL 的 wide mode 包含额外的 `si_errno` 和 pad 字段。

### System (SYSTEM class)

系统事件 (MMAP, MUNMAP, IPI, PAGEWAIT, TIMER, POWER 等) → **Instant** on CPU Track

31 种 SYSTEM 事件名称，29 种含完整的 fast/wide 字段布局（APS、MAPNAME、FUNC_ENTER/EXIT、IPI、PAGEWAIT、TIMER 等）。

### Flow Events

Flow 事件在 Perfetto 中显示为线程间的因果箭头。通过精确匹配算法在正向 dispatch 阶段内联生成，无额外内存开销。

| Flow 类型 | 起点 | 终点 | 匹配方式 |
|-----------|------|------|---------|
| `sync_mutex` | SyncMutexUnlock ENTER | 等待者 MUTEX→READY | sync_ptr 匹配 |
| `sync_condvar` | SyncCondvarSignal ENTER | 等待者 CONDVAR→READY | sync_ptr 匹配 |
| `sync_sem` | SyncSemPost ENTER | 等待者 SEM→READY | sync_ptr 匹配 |
| `ipc_msg` | Client MsgSend ENTER | Server MsgReceive EXIT | info_pid/info_tid |
| `ipc_send_ack` | Server MsgReceive EXIT | Client 确认 | 与 ipc_msg 同时生成 |
| `ipc_reply` | Server COMM REPLY | Client REPLY→READY | target_pid/target_tid |
| `signal` | SignalKill ENTER | SIGWAITINFO/SIGSUSPEND→READY | target_pid (0=self) |

IPC flow 形成完整的消息往返可视化：Client → Server (msg) → Client (ack) → Server处理 → Client (reply)。
Signal flow 追踪信号发送→接收因果关系（支持 QNX `pid=0` 自身信号约定）。

## 事件过滤

支持按时间范围、PID、TID 三个维度过滤输出事件，用于缩小分析范围和减小输出文件体积。

### 过滤参数

| 参数 | 格式 | 说明 |
|------|------|------|
| `--time-start` | `YYYY-MM-DD HH:MM:SS.nnnnnnnnn` | 起始时间（含），纳秒精度 |
| `--time-end` | `YYYY-MM-DD HH:MM:SS.nnnnnnnnn` | 结束时间（含） |
| `--pid` | 整数列表 | 过滤指定进程（可多个） |
| `--tid` | 整数列表 | 在 PID 内过滤指定线程（需配合 `--pid`） |

### 过滤行为

| 事件类型 | 时间过滤 | PID 过滤 | 说明 |
|----------|---------|---------|------|
| Thread State Slice | 适用 | 适用 | 仅输出匹配 PID/TID 的线程状态 |
| CPU Running Slice | 适用 | 适用 | 仅输出匹配线程在 CPU 上的运行片段 |
| KerCall Instant | 适用 | 适用 | 仅输出匹配线程的内核调用 |
| Comm Instant | 适用 | 适用 | 仅输出匹配线程的通信事件 |
| Interrupt Slice/Instant | 适用 | **不适用** | 中断事件始终保留 |
| System Instant | 适用 | **不适用** | 系统事件始终保留 |
| Flow Events | 适用 | 适用 | 至少一个端点匹配即保留 |
| Process Lifecycle | 适用 | 适用 | 父进程或子进程匹配即保留 |

**设计原则**: 过滤仅影响输出写入，内部状态跟踪（`_cpu_pending`、flow 匹配字典等）始终维护，确保过滤窗口边界处的事件正确闭合。

### 实测效果

基于 8650 平台 QST 文件（16.4s, 8.6M events）：

| 过滤条件 | 输出文件 | 缩减比 |
|----------|---------|--------|
| 无过滤 | 1,040 MB | — |
| 2s 时间窗口 | 118 MB | 88.6% |
| 单 PID (qcxserver) | 94 MB | 91.0% |
| 2s + PID + 3 TIDs | 9 MB | 99.1% |

## 时间戳计算

QNX trace 系统仅记录 32-bit `ClockCycles()` 低位，在 150MHz 下约 28.6 秒回绕一次。

解析器采用**反向锚定**策略：
1. `QstFileHeader.capture_end_ns` 作为绝对时间锚点
2. 从最后一个 event 向前，逐 event 计算 `signed_cycle_diff`
3. `signed_cycle_diff` 使用补码处理回绕

## 内存优化

| 优化策略 | 说明 |
|----------|------|
| mmap 文件访问 | 零拷贝，按需加载页面 |
| Buffer-by-buffer 处理 | 仅在内存中保留当前 buffer 的事件数据 |
| 即时写入 | 所有事件在 dispatch 时即时解析写入，无 deferred storage |
| `BatchingWriter` | protobuf 模式：10000 packets 一批写入 |
| `JsonTraceWriter` | JSON 模式：f-string 格式化 + 4096 事件缓冲 |
| `--lightweight` 模式 | 跳过 KerCall/Comm/System，仅处理 thread events |

## 依赖

- Python >= 3.8
- `perfetto` — Perfetto protobuf SDK（仅 protobuf 模式需要：`pip install perfetto`）
- JSON 模式无额外依赖

## 采集端配置 (tracer_engine.cc)

QST 文件的数据质量取决于采集端 (`tracer_engine.cc`) 的配置。关键配置项在 `deep_os_monitor_orin.json` 的 `tracer` 节中：

```json
"tracer": {
    "buffer_mode": "self-managed",
    "event_filter": "all",
    "buffer": { "size_mb": 500 }
}
```

### `event_filter` 选项

| 值 | 说明 | 事件类 | Wide Mode |
|----|------|--------|-----------|
| `"all"` | 全量采集 | `ADDALLCLASSES`（所有事件类） | 自动启用 |
| `"scheduling"` | 过滤采集 | 仅 Thread/Comm/KerCall/Process/Control | 自动启用 |

**两种模式都默认启用 Wide Mode**，确保 Thread 事件包含 `priority`/`policy`，KerCall/Comm/System/INT 事件包含完整参数字段。

### Wide Mode 数据效果

启用 Wide Mode 后，解析器输出中包含的信息：

| 事件类 | Fast Mode (旧) | Wide Mode (新) |
|--------|----------------|---------------|
| Thread | pid, tid | pid, tid, **priority**, **policy** |
| KerCall | 2 个参数 | **8 个**语义化参数 (coid, sparts, msg 等) |
| Comm | 2 个参数 | 完整 IPC 上下文 |
| INT | 中断号, IP | 中断号, IP, **area**, 完整 handler 信息 |

> **注意**: Wide Mode 会增加每个事件的数据量（从 2 个 data word 变为 4-8 个），可能导致 buffer 填充更快。建议根据采集时长适当调大 `buffer.size_mb`。

## 设计文档

详细设计方案参见 [`docs/parser_design_v4.md`](docs/parser_design_v4.md)。
