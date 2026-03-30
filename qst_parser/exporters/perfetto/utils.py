"""
Perfetto export utilities.

Provides a BatchingWriter that accumulates packets in memory and
flushes to disk in batches (default 10000 packets), giving near
in-memory speed with bounded memory.
"""

import uuid as uuid_module
from typing import Dict, Any, IO

from perfetto.protos.perfetto.trace.perfetto_trace_pb2 import Trace, TracePacket

TRUSTED_PACKET_SEQUENCE_ID = 12345

POLICY_NAMES: Dict[int, str] = {
    0: "UNKNOWN",
    1: "SCHED_FIFO",
    2: "SCHED_RR",
    3: "SCHED_OTHER",
    4: "SCHED_SPORADIC",
}


def format_policy(policy: int) -> str:
    return POLICY_NAMES.get(policy, f"POLICY_{policy}")


def generate_uuid() -> int:
    return uuid_module.uuid4().int & ((1 << 63) - 1)


class BatchingWriter:
    """Batching Perfetto trace writer.

    Accumulates packets in an in-memory Trace proto and flushes to disk
    every `batch_size` packets. Perfetto trace format allows concatenated
    Trace messages, so the output is a valid trace file.

    API is identical to TraceProtoBuilder (add_packet → TracePacket).
    """

    __slots__ = ('_file', '_trace', '_count', '_batch_size', '_total')

    def __init__(self, file: IO[bytes], batch_size: int = 10000):
        self._file = file
        self._trace = Trace()
        self._count = 0
        self._batch_size = batch_size
        self._total = 0

    def add_packet(self) -> TracePacket:
        if self._count >= self._batch_size:
            self._flush()
        self._count += 1
        self._total += 1
        return self._trace.packet.add()

    def _flush(self):
        if self._count > 0:
            self._file.write(self._trace.SerializeToString())
            self._trace.Clear()
            self._count = 0

    def finalize(self):
        self._flush()

    @property
    def total_packets(self) -> int:
        return self._total


def add_debug_annotations(track_event, args: Dict[str, Any]):
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


def write_slice(builder,
                start_ts: int,
                end_ts: int,
                track_uuid: int,
                name: str,
                args: Dict[str, Any] = None):
    """Write a duration slice (BEGIN + END) to the builder."""
    from perfetto.protos.perfetto.trace.perfetto_trace_pb2 import TrackEvent

    packet = builder.add_packet()
    packet.timestamp = start_ts
    packet.track_event.type = TrackEvent.TYPE_SLICE_BEGIN
    packet.track_event.track_uuid = track_uuid
    packet.track_event.name = name
    packet.trusted_packet_sequence_id = TRUSTED_PACKET_SEQUENCE_ID
    add_debug_annotations(packet.track_event, args)

    packet = builder.add_packet()
    packet.timestamp = end_ts
    packet.track_event.type = TrackEvent.TYPE_SLICE_END
    packet.track_event.track_uuid = track_uuid
    packet.trusted_packet_sequence_id = TRUSTED_PACKET_SEQUENCE_ID


def write_instant(builder,
                  ts: int,
                  track_uuid: int,
                  name: str,
                  args: Dict[str, Any] = None):
    """Write an instant event to the builder."""
    from perfetto.protos.perfetto.trace.perfetto_trace_pb2 import TrackEvent

    packet = builder.add_packet()
    packet.timestamp = ts
    packet.track_event.type = TrackEvent.TYPE_INSTANT
    packet.track_event.track_uuid = track_uuid
    packet.track_event.name = name
    packet.trusted_packet_sequence_id = TRUSTED_PACKET_SEQUENCE_ID
    add_debug_annotations(packet.track_event, args)
