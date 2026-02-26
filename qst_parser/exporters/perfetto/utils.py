"""
Perfetto 导出器工具函数
"""

import uuid as uuid_module
from typing import Dict, Any, TYPE_CHECKING

if TYPE_CHECKING:
    from perfetto.trace_builder.proto_builder import TraceProtoBuilder

# 常量
TRUSTED_PACKET_SEQUENCE_ID = 12345

# QNX 调度策略名称映射
POLICY_NAMES: Dict[int, str] = {
    0: "UNKNOWN",
    1: "SCHED_FIFO",
    2: "SCHED_RR",
    3: "SCHED_OTHER",
    4: "SCHED_SPORADIC",
}


def format_policy(policy: int) -> str:
    """将调度策略数字映射为可读名称"""
    return POLICY_NAMES.get(policy, f"POLICY_{policy}")


def generate_uuid() -> int:
    """生成 64 位 UUID"""
    return uuid_module.uuid4().int & ((1 << 63) - 1)


def ns_to_us(ns: int) -> int:
    """纳秒转微秒，不强制最小值"""
    return ns // 1000  # 如果 < 1000 ns，返回 0，由调用方决定是否跳过


def add_debug_annotations(track_event, args: Dict[str, Any]):
    """向 TrackEvent 添加 debug annotations"""
    if not args:
        return
    
    for key, value in args.items():
        ann = track_event.debug_annotations.add()
        ann.name = key
        if isinstance(value, int):
            ann.int_value = value
        elif isinstance(value, float):
            ann.double_value = value
        else:
            ann.string_value = str(value)


def write_slice(builder: "TraceProtoBuilder", 
                start_ts: int, 
                end_ts: int,
                track_uuid: int, 
                name: str, 
                args: Dict[str, Any] = None):
    """
    写入 Duration Slice (BEGIN + END)
    
    使用官方推荐的 BEGIN/END 方式，时间戳是纳秒精度。
    Perfetto 会自动根据时间戳排序和匹配 BEGIN/END。
    
    注意：同一个 Track 上的 slice 必须正确嵌套或不重叠，
    否则 BEGIN/END 会错误配对。
    
    Args:
        builder: TraceProtoBuilder 实例
        start_ts: 开始时间戳（纳秒）
        end_ts: 结束时间戳（纳秒）
        track_uuid: Track UUID
        name: 事件名称
        args: 可选的 debug annotations
    """
    from perfetto.protos.perfetto.trace.perfetto_trace_pb2 import TrackEvent
    
    # BEGIN
    packet = builder.add_packet()
    packet.timestamp = start_ts
    packet.track_event.type = TrackEvent.TYPE_SLICE_BEGIN
    packet.track_event.track_uuid = track_uuid
    packet.track_event.name = name
    packet.trusted_packet_sequence_id = TRUSTED_PACKET_SEQUENCE_ID
    add_debug_annotations(packet.track_event, args)
    
    # END
    packet = builder.add_packet()
    packet.timestamp = end_ts
    packet.track_event.type = TrackEvent.TYPE_SLICE_END
    packet.track_event.track_uuid = track_uuid
    packet.trusted_packet_sequence_id = TRUSTED_PACKET_SEQUENCE_ID


def write_instant(builder: "TraceProtoBuilder",
                  ts: int,
                  track_uuid: int,
                  name: str,
                  args: Dict[str, Any] = None):
    """
    写入 Instant 事件
    
    Args:
        builder: TraceProtoBuilder 实例
        ts: 时间戳（纳秒）
        track_uuid: Track UUID
        name: 事件名称
        args: 可选的 debug annotations
    """
    from perfetto.protos.perfetto.trace.perfetto_trace_pb2 import TrackEvent
    
    packet = builder.add_packet()
    packet.timestamp = ts
    packet.track_event.type = TrackEvent.TYPE_INSTANT
    packet.track_event.track_uuid = track_uuid
    packet.track_event.name = name
    packet.trusted_packet_sequence_id = TRUSTED_PACKET_SEQUENCE_ID
    add_debug_annotations(packet.track_event, args)
