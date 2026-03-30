"""
Chrome JSON Trace Event Format writer for Perfetto.

Writes events in the Chrome Trace Event Format (JSON) which Perfetto UI
can open directly.  This avoids all Python protobuf overhead -- events are
serialized as plain text and streamed to disk.

Hot-path methods (write_complete, write_instant) use pre-formatted strings
instead of json.dump to minimise per-event overhead.

Format reference:
  https://docs.google.com/document/d/1CvAClvFfyA5R-PhYUmn5OOQtYMH4h6I0nSsKchNAySU

Supported event phases:
  X  -- Complete event (duration slice)
  i  -- Instant event (thread scope)
  M  -- Metadata (process_name, thread_name)
"""

from typing import Dict, Any, IO, Optional

_CPU_PID = 999000


def _esc(s: str) -> str:
    """JSON-escape a string value (without surrounding quotes)."""
    if '\\' not in s and '"' not in s and '\n' not in s:
        return s
    return (s.replace('\\', '\\\\')
             .replace('"', '\\"')
             .replace('\n', '\\n')
             .replace('\r', '\\r')
             .replace('\t', '\\t')
             .replace('\b', '\\b')
             .replace('\f', '\\f'))


def _format_args(args: dict) -> str:
    """Format a flat dict as JSON object. ~3-5x faster than json.dumps for
    dicts of str/int values (no nested structures, no encoder overhead)."""
    parts = []
    for k, v in args.items():
        if isinstance(v, str):
            parts.append(f'"{k}":"{_esc(v)}"')
        elif isinstance(v, bool):
            parts.append(f'"{k}":{"true" if v else "false"}')
        else:
            parts.append(f'"{k}":{v}')
    return '{' + ','.join(parts) + '}'


class JsonTraceWriter:
    """Streaming Chrome JSON trace writer.

    Timestamps are in **microseconds** (Perfetto JSON convention).
    """

    __slots__ = ('_f', '_first', '_total', '_w', '_buf', '_buf_n')

    _FLUSH_THRESHOLD = 4096

    def __init__(self, file: IO[str]):
        self._f = file
        self._w = file.write
        self._first = True
        self._total = 0
        self._buf: list = []
        self._buf_n = 0
        self._w('[')

    def _sep(self) -> str:
        if self._first:
            self._first = False
            return ''
        return ','

    def _flush_buf(self):
        if self._buf:
            self._w(''.join(self._buf))
            self._buf.clear()
            self._buf_n = 0

    def write_process_name(self, pid: int, name: str):
        self._buf.append(
            f'{self._sep()}{{"ph":"M","pid":{pid},"tid":0,'
            f'"name":"process_name","args":{{"name":"{_esc(name)}"}}}}'
        )
        self._buf_n += 1

    def write_thread_name(self, pid: int, tid: int, name: str):
        self._buf.append(
            f'{self._sep()}{{"ph":"M","pid":{pid},"tid":{tid},'
            f'"name":"thread_name","args":{{"name":"{_esc(name)}"}}}}'
        )
        self._buf_n += 1

    def write_process_sort_index(self, pid: int, sort_index: int):
        self._buf.append(
            f'{self._sep()}{{"ph":"M","pid":{pid},"tid":0,'
            f'"name":"process_sort_index","args":{{"sort_index":{sort_index}}}}}'
        )
        self._buf_n += 1

    def write_complete(self, pid: int, tid: int,
                       start_us: float, dur_us: float,
                       name: str, cat: str,
                       args: Optional[Dict[str, Any]] = None):
        if args:
            self._buf.append(
                f'{self._sep()}{{"ph":"X","pid":{pid},"tid":{tid},'
                f'"ts":{start_us:.3f},"dur":{dur_us:.3f},'
                f'"name":"{_esc(name)}","cat":"{cat}","args":{_format_args(args)}}}'
            )
        else:
            self._buf.append(
                f'{self._sep()}{{"ph":"X","pid":{pid},"tid":{tid},'
                f'"ts":{start_us:.3f},"dur":{dur_us:.3f},'
                f'"name":"{_esc(name)}","cat":"{cat}"}}'
            )
        self._total += 1
        self._buf_n += 1
        if self._buf_n >= self._FLUSH_THRESHOLD:
            self._flush_buf()

    def write_instant(self, pid: int, tid: int, ts_us: float,
                      name: str, cat: str,
                      args: Optional[Dict[str, Any]] = None):
        if args:
            self._buf.append(
                f'{self._sep()}{{"ph":"i","pid":{pid},"tid":{tid},'
                f'"ts":{ts_us:.3f},"s":"t",'
                f'"name":"{_esc(name)}","cat":"{cat}","args":{_format_args(args)}}}'
            )
        else:
            self._buf.append(
                f'{self._sep()}{{"ph":"i","pid":{pid},"tid":{tid},'
                f'"ts":{ts_us:.3f},"s":"t",'
                f'"name":"{_esc(name)}","cat":"{cat}"}}'
            )
        self._total += 1
        self._buf_n += 1
        if self._buf_n >= self._FLUSH_THRESHOLD:
            self._flush_buf()

    def write_flow_start(self, pid: int, tid: int, ts_us: float,
                         flow_id: int, name: str, cat: str):
        """Emit a flow start event (ph='s') at the wait point."""
        self._buf.append(
            f'{self._sep()}{{"ph":"s","pid":{pid},"tid":{tid},'
            f'"ts":{ts_us:.3f},"id":{flow_id},'
            f'"name":"{_esc(name)}","cat":"{cat}"}}'
        )
        self._total += 1
        self._buf_n += 1
        if self._buf_n >= self._FLUSH_THRESHOLD:
            self._flush_buf()

    def write_flow_end(self, pid: int, tid: int, ts_us: float,
                       flow_id: int, name: str, cat: str):
        """Emit a flow end event (ph='f', bp='e') at the wake point."""
        self._buf.append(
            f'{self._sep()}{{"ph":"f","pid":{pid},"tid":{tid},'
            f'"ts":{ts_us:.3f},"id":{flow_id},"bp":"e",'
            f'"name":"{_esc(name)}","cat":"{cat}"}}'
        )
        self._total += 1
        self._buf_n += 1
        if self._buf_n >= self._FLUSH_THRESHOLD:
            self._flush_buf()

    def finalize(self):
        self._flush_buf()
        self._w(']')

    @property
    def total_events(self) -> int:
        return self._total

    @staticmethod
    def cpu_pid() -> int:
        """All CPU tracks share a single virtual process."""
        return _CPU_PID

    @staticmethod
    def cpu_main_tid(cpu_id: int) -> int:
        """Thread id for the main scheduling track of a given CPU."""
        return cpu_id * 10 + 1

    @staticmethod
    def cpu_irq_tid(cpu_id: int) -> int:
        """Thread id for the IRQ sub-track of a given CPU."""
        return cpu_id * 10 + 2
