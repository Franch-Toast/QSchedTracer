# QST Parser v2.0

模块化的 QNX Trace Event 解析器，用于解析 QSchedTracer 生成的 .qst 文件。

## 架构设计

### 核心原则

1. **按事件类别组织**：每种 trace class 对应一个独立目录
2. **细粒度事件解析**：每个 event 类型有独立的解析器，支持 Fast/Wide mode
3. **关注点分离**：parser.py 只做高层调度，具体解析逻辑在各 class 中
4. **常量本地化**：事件相关常量放在各自 class 的 constants.py 中

### 项目结构

```
qst_parser/
├── __init__.py              # 包入口
├── __main__.py              # 模块执行入口
├── main.py                  # 命令行入口
├── parser.py                # 主解析器 (只做高层调度)
├── constants.py             # 公共常量 (InternalClass, StructType 等)
│
├── models/                  # 公共数据模型
│   ├── base.py              # RawEvent, BaseEvent
│   └── headers.py           # FileHeader, ProcInfoHeader, MainDataHeader
│
├── utils/                   # 工具函数
│   ├── timestamp.py         # 时间戳计算
│   └── converters.py        # 事件类型转换
│
├── classes/                 # 事件类别处理器
│   ├── base.py              # BaseClassHandler 基类
│   │
│   ├── thread/              # _NTO_TRACE_THREAD
│   │   ├── constants.py     # ThreadState, THREAD_STATES
│   │   ├── models.py        # ThreadEvent
│   │   ├── handler.py       # ThreadClassHandler
│   │   └── events/          # 事件解析器
│   │       └── state_event.py
│   │
│   ├── vthread/             # _NTO_TRACE_VTHREAD
│   │   └── handler.py       # 复用 thread 的解析逻辑
│   │
│   ├── process/             # _NTO_TRACE_PROCESS
│   │   ├── constants.py     # ProcessEventType
│   │   ├── models.py        # ProcessCreateEvent, ProcessDestroyEvent, ...
│   │   ├── handler.py       # ProcessClassHandler
│   │   └── events/
│   │       ├── proccreate.py
│   │       ├── procdestroy.py
│   │       └── procthread_name.py
│   │
│   ├── kercall/             # _NTO_TRACE_KERCALL*
│   │   ├── constants.py     # KernelCall, SIGNAL_NAMES
│   │   ├── models.py        # SignalKillEvent, MsgSendEvent, ...
│   │   ├── handler.py       # KerCallClassHandler (含时间戳计算、发送者推断)
│   │   └── events/
│   │       └── signal_kill.py  # Fast/Wide mode 解析
│   │
│   ├── control/             # _NTO_TRACE_CONTROL
│   │   ├── constants.py
│   │   └── handler.py
│   │
│   ├── interrupt/           # _NTO_TRACE_INT*
│   │   ├── constants.py
│   │   ├── models.py
│   │   ├── handler.py
│   │   └── events/
│   │       ├── int_enter.py
│   │       ├── int_exit.py
│   │       └── int_handler.py
│   │
│   ├── comm/                # _NTO_TRACE_COMM
│   │   ├── constants.py
│   │   ├── models.py
│   │   ├── handler.py
│   │   └── events/
│   │       ├── msg.py
│   │       ├── pulse.py
│   │       └── signal.py
│   │
│   └── system/              # _NTO_TRACE_SYSTEM
│       ├── constants.py
│       ├── models.py
│       ├── handler.py
│       └── events/
│           ├── mmap.py
│           └── func_trace.py
│
└── exporters/               # 导出器
    └── perfetto.py          # Perfetto JSON 导出
```

## 使用方法

### 命令行

```bash
# 显示摘要
python -m qst_parser trace.qst

# 导出 Perfetto JSON
python -m qst_parser trace.qst -o trace.json

# 详细输出
python -m qst_parser trace.qst -v
```

### Python API

```python
from qst_parser import QstParser

# 解析文件
parser = QstParser("trace.qst", verbose=True)
parser.parse()

# 打印摘要
parser.print_summary()

# 访问数据
print(f"线程事件数: {len(parser.thread_events)}")
print(f"信号事件数: {len(parser.signal_events)}")

# 导出 Perfetto JSON
parser.to_perfetto_json("trace.json")
```

## 事件类别 (Classes)

### Thread (_NTO_TRACE_THREAD)

线程状态变化事件：
- DEAD, RUNNING, READY, STOPPED
- SEND, RECEIVE, REPLY
- MUTEX, CONDVAR, SEM, NANOSLEEP
- 等等

### Process (_NTO_TRACE_PROCESS)

进程相关事件：
- PROCCREATE / PROCCREATE_NAME
- PROCDESTROY / PROCDESTROY_NAME
- PROCTHREAD_NAME

### KerCall (_NTO_TRACE_KERCALL*)

内核调用事件（已实现）：
- SIGNAL_KILL: 信号发送

### Interrupt (_NTO_TRACE_INT*)

中断事件：
- INTENTER / INTEXIT
- INT_HANDLER_ENTER / INT_HANDLER_EXIT

### Comm (_NTO_TRACE_COMM)

通信事件：
- SMSG / RMSG / REPLY / ERROR
- SPULSE / RPULSE
- SIGNAL

### System (_NTO_TRACE_SYSTEM)

系统事件：
- MMAP / MUNMAP
- FUNC_ENTER / FUNC_EXIT
- 等等

## Fast mode vs Wide mode

QNX trace 系统支持两种事件记录模式：

### Fast mode (SIMPLE)
- 使用 `StructType.SIMPLE`
- 精简参数，较低开销
- 一个事件 = 16 字节

### Wide mode (COMBINE)
- 使用 `StructType.COMBINE_BEGIN/CONT/END`
- 完整参数，使用组合事件传输
- 多个 16 字节事件组成一个逻辑事件
- 通过时间戳匹配组合

示例 (SignalKill Wide mode)：
```
COMBINE_BEGIN: timestamp, nd, pid
COMBINE_CONT:  timestamp, tid, signo
COMBINE_END:   timestamp, code, value
```

## 扩展指南

### 添加新的事件类别

1. 在 `classes/` 下创建新目录
2. 创建 `constants.py` 定义事件类型常量
3. 创建 `models.py` 定义数据结构
4. 创建 `events/` 目录，为每种事件创建解析器
5. 创建 `handler.py` 实现 `BaseClassHandler`
6. 在 `classes/__init__.py` 中注册
7. 在 `parser.py` 的 `_dispatch_event` 中添加分发逻辑

### 添加新的事件解析器

1. 在对应 class 的 `events/` 目录创建解析器文件
2. 实现 Fast mode 和 Wide mode 解析
3. 在 `events/__init__.py` 中导出
4. 在 handler 中调用

## 参考文档

- QNX System Analysis Toolkit Documentation
- kercall_table_Events.html - 所有事件的数据格式定义
- events_Classes.html - 事件类别列表
- [Perfetto Trace Format](https://perfetto.dev/docs/reference/trace-format)
