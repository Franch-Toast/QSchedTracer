#!/usr/bin/env python3
"""
QST Parser - 命令行入口

QNX 调度追踪数据解析工具 (v2.0.0)

功能:
    1. 解析 .qst 二进制文件 (v3 格式)
    2. 从事件 header 提取 CPU ID
    3. 从 PROCDESTROY 事件提取进程/线程名称
    4. 计算真实 UNIX 时间戳 (从最后一个事件向前推算)
    5. 导出为 Perfetto protobuf 格式 (.perfetto-trace)
    6. 检查缓冲区连续性 (buffer_seq)

用法:
    qst_parser trace.qst                        # 解析并导出 trace.perfetto
    qst_parser trace.qst -o custom.perfetto     # 指定输出文件名
    qst_parser trace.qst --no-export            # 仅显示摘要，不导出
    qst_parser trace.qst -v                     # 详细输出
"""

import os
import sys
import argparse

from .parser import QstParser
from .exporters import PerfettoExporter


def _print_data_integrity_check(qst):
    """打印数据完整性检查结果"""
    print(f"\n数据完整性检查:")
    
    # 检查进程/线程名称
    proc_count = len(qst.process_info)
    thread_count = len(qst.thread_info)
    
    # 统计唯一的进程和线程
    thread_events = qst.thread_events
    unique_pids = set(e.pid for e in thread_events)
    unique_threads = set((e.pid, e.tid) for e in thread_events)
    
    print(f"  进程名称: {proc_count}/{len(unique_pids)} "
          f"({'完整' if proc_count >= len(unique_pids) else '不完整 - 采集器未收集进程名称'})")
    print(f"  线程名称: {thread_count}/{len(unique_threads)} "
          f"({'完整' if thread_count >= len(unique_threads) else '不完整'})")
    
    # 检查 MUTEX 状态与 MutexLock 的对应关系
    if hasattr(qst, '_kercall_handler') and qst._kercall_handler:
        kercall = qst._kercall_handler
        mutex_locks = len([e for e in kercall.mutex_events 
                         if type(e).__name__ == 'MutexLockEnterEvent'])
        mutex_states = len([e for e in thread_events if e.state == 'MUTEX'])
        
        if mutex_states > mutex_locks:
            diff = mutex_states - mutex_locks
            print(f"  MUTEX 状态: {mutex_states}, MutexLock ENTER: {mutex_locks} "
                  f"(差异 {diff} - 可能部分线程在采集前已阻塞)")
        else:
            print(f"  MUTEX 状态: {mutex_states}, MutexLock ENTER: {mutex_locks} (正常)")
    
    # 检查 ProcInfo 中的 PROCESS 事件
    if qst.procinfo_header and qst.procinfo_header.event_count > 0:
        print(f"  ProcInfo 事件数: {qst.procinfo_header.event_count}")
        if proc_count < len(unique_pids) // 2:  # 少于一半的进程有名称
            print(f"  [警告] ProcInfo 中缺少 PROCESS 类事件，大部分进程/线程名称未知")
            print(f"         原因：_NTO_TRACE_START 只注入状态快照，不包含名称")
            print(f"         建议：在采集器中添加通过 /proc 查询进程名称的功能")


def _get_default_output(input_path: str) -> str:
    """
    根据输入文件路径生成默认输出文件名
    
    将输入文件的扩展名替换为 .perfetto
    例如: tracer_20260204.qst -> tracer_20260204.perfetto
    """
    base, _ = os.path.splitext(input_path)
    return f"{base}.perfetto"


def main():
    """命令行主入口"""
    parser = argparse.ArgumentParser(
        description='QST 文件解析器 - QNX 调度追踪数据解析工具 (v2.0.0)',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
示例:
  %(prog)s trace.qst                        解析并导出 trace.perfetto
  %(prog)s trace.qst -o custom.perfetto     指定输出文件名
  %(prog)s trace.qst --no-export            仅显示摘要，不导出
  %(prog)s trace.qst -v                     详细输出

输出格式:
  .perfetto - Perfetto protobuf 格式
  直接在 https://ui.perfetto.dev 中打开查看
""")
    
    parser.add_argument('input', help='输入 .qst 文件')
    parser.add_argument('-o', '--output', help='输出文件路径 (默认: 输入文件同名.perfetto)')
    parser.add_argument('--no-export', action='store_true', help='仅解析显示摘要，不导出 Perfetto 文件')
    parser.add_argument('-v', '--verbose', action='store_true', help='详细输出')
    
    args = parser.parse_args()
    
    try:
        qst = QstParser(args.input, verbose=args.verbose)
        qst.parse()
        qst.print_summary()
        
        # 打印缓冲区连续性检查结果
        if hasattr(qst, '_control_handler') and qst._control_handler:
            summary = qst._control_handler.get_summary()
            print(f"\n缓冲区连续性检查:")
            print(f"  总缓冲区数: {summary['total_buffers']}")
            print(f"  不连续次数: {summary['discontinuities']}")
            print(f"  状态: {'正常' if summary['is_continuous'] else '存在丢失'}")
        
        # 打印数据完整性检查
        _print_data_integrity_check(qst)
        
        # 导出 Perfetto 文件（默认导出，除非指定 --no-export）
        if not args.no_export:
            output_path = args.output if args.output else _get_default_output(args.input)
            
            print(f"\n导出 Perfetto 格式: {output_path}")
            exporter = PerfettoExporter(qst)
            stats = exporter.export(output_path)
            
            print(f"\n导出统计:")
            print(f"  CPU Tracks: {stats['cpu_tracks']}")
            print(f"  Process Tracks: {stats['process_tracks']}")
            print(f"  Thread Tracks: {stats['thread_tracks']}")
            print(f"  Thread State Slices: {stats['thread_state_slices']}")
            print(f"  CPU Running Slices: {stats['cpu_running_slices']}")
            print(f"  Interrupt Slices: {stats['interrupt_slices']}")
            print(f"  Interrupt Instants: {stats['interrupt_instants']}")
            print(f"  Sync Enter Instants: {stats['sync_enter']}")
            print(f"  Sync Exit Instants: {stats['sync_exit']}")
            print(f"  IPC Slices: {stats['ipc_slices']}")
            print(f"  IPC Instants: {stats['ipc_instants']}")
            print(f"  Sched Instants: {stats['sched_instants']}")
            print(f"  文件大小: {stats['file_size_kb']:.2f} KB")
            print(f"\n打开 https://ui.perfetto.dev 导入 {output_path} 查看")
    
    except FileNotFoundError:
        print(f"错误: 文件不存在: {args.input}", file=sys.stderr)
        sys.exit(1)
    except Exception as e:
        print(f"错误: {e}", file=sys.stderr)
        if args.verbose:
            import traceback
            traceback.print_exc()
        sys.exit(1)


if __name__ == '__main__':
    main()
