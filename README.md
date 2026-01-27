# QSchedTracer

**QNX 调度追踪器** - 轻量级飞行记录仪模式事件采集工具 (v2.0)

## 概述

QSchedTracer 是专为 QNX 7.1 Neutrino RTOS 设计的系统追踪工具。采用"飞行记录仪"模式持续记录调度事件，支持：

- **配置驱动**：JSON 配置文件控制事件过滤和触发条件
- **可扩展触发**：支持内核事件触发落盘 (如 SIGKILL)
- **真实时间戳**：支持 UNIX 时间戳转换，精确到微秒
- **Perfetto 集成**：导出标准 Chrome Trace Event 格式

**v2.0 新特性**：

- 模块化架构重构 (配置/触发器/事件/数据管理器)
- JSON 配置文件支持
- `RingBuffer` → `DataBuffer` 重命名
- 固定输出文件名 `trace_{timestamp}.qst`

## 快速开始

```bash
# 1. 编译
./build.bash

# 2. 部署到 QNX 设备
scp build/qnx_aarch64/qst_tracer root@<QNX_IP>:/tmp/
scp config/default.json root@<QNX_IP>:/tmp/qst_config.json

# 3. 运行采集 (使用默认配置)
ssh root@<QNX_IP> '/tmp/qst_tracer'     # Ctrl+C 停止

# 4. 使用指定配置
ssh root@<QNX_IP> '/tmp/qst_tracer -c /tmp/qst_config.json'

# 5. 取回数据并解析
scp root@<QNX_IP>:/tmp/trace_*.qst .
python3 tools/qst_parse.py trace_*.qst -o trace.json

# 6. 可视化: 打开 https://ui.perfetto.dev/ 并拖入 trace.json
```

## 架构

### 系统架构

```
┌──────────────────────────────────────────────────────────────────────┐
│                              应用层                                   │
│  ┌────────────┐  ┌────────────────┐  ┌────────────┐                 │
│  │    CLI     │  │  ConfigLoader  │  │   Logger   │                 │
│  │ (main.cpp) │  │  (JSON 解析)   │  │  (spdlog)  │                 │
│  └─────┬──────┘  └───────┬────────┘  └────────────┘                 │
│        │                 │                                           │
│        └────────┬────────┘                                           │
│                 ▼                                                     │
└──────────────────────────────────────────────────────────────────────┘
                  │
                  ▼
┌──────────────────────────────────────────────────────────────────────┐
│                             核心层                                    │
│  ┌────────────────────────────────────────────────────────────────┐ │
│  │                        TracerEngine                             │ │
│  │  ┌──────────────┐  ┌──────────────┐  ┌──────────────────────┐ │ │
│  │  │ EventManager │  │TriggerManager│  │    DataManager       │ │ │
│  │  │ (事件配置)    │  │ (触发器管理)  │  │ (落盘 + 进程信息)    │ │ │
│  │  └──────────────┘  └──────────────┘  └──────────────────────┘ │ │
│  │                           │                                    │ │
│  │                           ▼                                    │ │
│  │  ┌─────────────────────────────────────────────────────────┐  │ │
│  │  │                    DataBuffer                            │  │ │
│  │  │  (环形缓冲区，持续记录，触发时快照保存)                   │  │ │
│  │  └─────────────────────────────────────────────────────────┘  │ │
│  └────────────────────────────────────────────────────────────────┘ │
└──────────────────────────────────────────────────────────────────────┘
```

### 数据流

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
│  DataBuffer (10MB, 环形覆盖)                                        │
│     │  - 自动覆盖旧数据，保留最近 N 秒                              │
│     │                                                               │
│     │ 触发落盘 (Ctrl+C / 触发器条件)                                │
│     ▼                                                               │
│  1. _NTO_TRACE_STOP + FLUSHBUFFER                                   │
│  2. 记录真实时间同步点 (wallclock + cycles)                         │
│  3. 使用独立缓冲区采集进程/线程名称                                 │
│  4. 写入 trace_{timestamp}.qst                                      │
└─────────────────────────────────────────────────────────────────────┘
      │
      ▼
┌─────────────────────────────────────────────────────────────────────┐
│                       离线处理                                      │
│                                                                     │
│  trace_*.qst ──▶ qst_parse.py ──▶ trace.json ──▶ Perfetto UI        │
└─────────────────────────────────────────────────────────────────────┘
```

## 配置文件

### 默认配置 (config/default.json)

```json
{
  "version": "1.0",
  "buffer": {
    "size_mb": 10
  },
  "scheduling": {
    "enabled": true,
    "mode": "fast"
  },
  "extra_events": {
    "classes": [],
    "specific_events": [
      {
        "class": 3,
        "event": 26,
        "mode": "wide",
        "comment": "KERCALLENTER: __KER_SIGNAL_KILL"
      }
    ]
  },
  "triggers": [
    {
      "type": "kernel_event",
      "class": 3,
      "event": 26,
      "condition": {
        "data_index": 3,
        "op": "eq",
        "value": 9
      },
      "comment": "data[3]=signo, 9=SIGKILL"
    }
  ]
}
```

### 配置项说明

| 配置项 | 类型 | 说明 |
|--------|------|------|
| `buffer.size_mb` | int | 缓冲区大小 (MB) |
| `scheduling.enabled` | bool | 是否启用调度追踪 |
| `scheduling.mode` | string | "fast" 或 "wide" |
| `extra_events.classes` | array | 额外的事件类 |
| `extra_events.specific_events` | array | 特定事件 |
| `triggers` | array | 触发器配置 |

详见 `docs/architecture_redesign.md`

## 编译

### 环境要求

| 依赖 | 版本 | 说明 |
|------|------|------|
| QNX SDP | 7.1+ | 交叉编译工具链 |
| C++ | C++17 | 语言标准 |
| spdlog | 1.15+ | 日志库 |
| nlohmann/json | 3.11+ | JSON 解析库 |
| Python | 3.6+ | 解析器运行环境 |

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
mkdir cmake_build && cd cmake_build
cmake -DCMAKE_TOOLCHAIN_FILE=../cmake/qnx710-aarch64.cmake ..
make
```

### 方法三：使用 Bazel

```bash
# 构建可执行文件 (QNX SA8650 平台)
bazel build --config=sa8650_qnx //:qst_tracer_bin

# 构建完整发布包
bazel build --config=sa8650_qnx //:qst_tracer_release_package
```

## 命令行接口

```
qst_tracer [选项]

选项:
  -c <文件>  配置文件路径 (默认: ./qst_config.json)
  -b <MB>    缓冲区大小，覆盖配置 (默认: 配置文件值或 10)
  -v         详细输出
  -h         显示帮助

输出文件:
  trace_YYYYMMDD_HHMMSS.qst  (固定格式)

示例:
  qst_tracer                          # 使用默认配置
  qst_tracer -c /etc/qst/config.json  # 使用指定配置
  qst_tracer -b 16                    # 覆盖缓冲区大小
```

## 项目结构

```
QSchedTracer/
├── build.bash                      # 编译脚本
├── CMakeLists.txt                  # CMake 配置
├── BUILD                           # Bazel 配置
├── README.md                       # 本文档
│
├── config/
│   └── default.json                # 默认配置
│
├── include/qst/
│   ├── types.hpp                   # 公共类型定义
│   ├── log.hpp                     # 日志工具
│   │
│   ├── config/
│   │   ├── types.hpp               # 配置数据结构
│   │   └── config_loader.hpp       # 配置加载器
│   │
│   ├── core/
│   │   ├── data_buffer.hpp         # 数据缓冲区
│   │   └── tracer_engine.hpp       # 追踪引擎
│   │
│   ├── trigger/
│   │   ├── trigger.hpp             # 触发器接口
│   │   ├── trigger_manager.hpp     # 触发器管理器
│   │   └── kernel_event_trigger.hpp # 内核事件触发器
│   │
│   ├── event/
│   │   └── event_manager.hpp       # 事件管理器
│   │
│   └── data/
│       └── data_manager.hpp        # 数据管理器
│
├── src/
│   ├── main.cpp                    # 主入口
│   │
│   ├── config/
│   │   └── config_loader.cpp
│   │
│   ├── core/
│   │   ├── data_buffer.cpp
│   │   └── tracer_engine.cpp
│   │
│   ├── trigger/
│   │   ├── kernel_event_trigger.cpp
│   │   └── trigger_manager.cpp
│   │
│   ├── event/
│   │   └── event_manager.cpp
│   │
│   └── data/
│       └── data_manager.cpp
│
├── spdlog/                         # spdlog (header-only)
├── third_party/
│   └── nlohmann/
│       └── json.hpp                # nlohmann/json
│
├── tools/
│   └── qst_parse.py                # Python 解析器
│
└── docs/
    ├── architecture_redesign.md    # 架构设计文档
    └── trace_event_format.md       # Perfetto 格式参考
```

## 技术细节

### 时间戳处理

QNX trace 事件使用 32 位 cycles 时间戳，约 3-4 秒回绕一次。采用"结束同步点"方案：

1. 采集结束时记录 `wallclock` (CLOCK_REALTIME) + `sync_cycles` (最后事件的 cycles)
2. 解析器从最后一个事件向前推算，自动处理回绕
3. 真实时间 = wallclock - (sync_cycles - event_cycles) / clock_freq

### 进程/线程名称采集

1. 主采集使用 `_NTO_TRACE_STARTNOSTATE` (不注入初始状态)
2. 落盘前使用**独立缓冲区**调用 `_NTO_TRACE_START` 采集名称
3. 通过 `g_active_buffer` 指针动态切换，避免污染主调度数据

### 性能特性

| 指标 | 数值 |
|------|------|
| 中断处理时间 | ~2-4 µs (memcpy 16KB) |
| 中断频率 | ~100-500 Hz |
| CPU 开销 | < 0.2% |
| 事件容量 | ~625K 事件/10MB |

## License

MIT License
