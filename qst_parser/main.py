#!/usr/bin/env python3
"""
QST Parser — Streaming Parser for QST v4 (new format)

Usage:
    python -m qst_parser trace.qst
    python -m qst_parser trace.qst -o output.perfetto
    python -m qst_parser trace.qst --format json -o output.json
    python -m qst_parser trace.qst --no-export
    python -m qst_parser trace.qst --lightweight
"""

import os
import sys
import time
import argparse
from datetime import datetime, timezone


def parse_timestamp(ts_str: str) -> int:
    """Parse '1970-01-06 02:37:34.751884416' into nanoseconds since epoch."""
    parts = ts_str.strip().split('.')
    dt = datetime.strptime(parts[0], '%Y-%m-%d %H:%M:%S')
    dt = dt.replace(tzinfo=timezone.utc)
    base_ns = int(dt.timestamp()) * 1_000_000_000
    if len(parts) > 1:
        frac = parts[1].ljust(9, '0')[:9]
        base_ns += int(frac)
    return base_ns


def main():
    parser = argparse.ArgumentParser(
        description='QST Parser — Streaming QNX trace parser (QST v4 format)',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  %(prog)s trace.qst                              Parse and export trace.perfetto
  %(prog)s trace.qst -o custom.perfetto           Custom output path
  %(prog)s trace.qst --format json                Export as Chrome JSON
  %(prog)s trace.qst --format json -o trace.json  JSON with custom path
  %(prog)s trace.qst --no-export                  Parse summary only
  %(prog)s trace.qst --lightweight                Thread events only (fast, low memory)
""")

    parser.add_argument('input', help='Input .qst file')
    parser.add_argument('-o', '--output', help='Output file path')
    parser.add_argument('--format', choices=['protobuf', 'json'],
                        default='protobuf',
                        help='Output format (default: protobuf)')
    parser.add_argument('--no-export', action='store_true',
                        help='Parse summary only, do not export')
    parser.add_argument('--lightweight', action='store_true',
                        help='Lightweight mode: only thread/CPU events')
    parser.add_argument('-v', '--verbose', action='store_true',
                        help='Verbose output')

    filt = parser.add_argument_group('filtering',
                                     'Filter output by time range or process/thread')
    filt.add_argument('--time-start',
                      help='Start time (e.g. "1970-01-06 02:37:34.751884416")')
    filt.add_argument('--time-end',
                      help='End time (e.g. "1970-01-06 02:37:48.000000000")')
    filt.add_argument('--pid', type=int, nargs='+', metavar='PID',
                      help='Filter by process ID(s)')
    filt.add_argument('--tid', type=int, nargs='+', metavar='TID',
                      help='Filter by thread ID(s) (within --pid processes)')

    args = parser.parse_args()

    if args.tid and not args.pid:
        parser.error('--tid requires --pid')

    if not os.path.isfile(args.input):
        print(f"Error: file not found: {args.input}", file=sys.stderr)
        sys.exit(1)

    from .core.streaming_parser import StreamingParser

    filter_time_start = parse_timestamp(args.time_start) if args.time_start else None
    filter_time_end = parse_timestamp(args.time_end) if args.time_end else None
    filter_pids = frozenset(args.pid) if args.pid else None
    filter_tids = frozenset(args.tid) if args.tid else None

    sp = StreamingParser(
        args.input, verbose=args.verbose, lightweight=args.lightweight,
        filter_pids=filter_pids, filter_tids=filter_tids,
        filter_time_start_ns=filter_time_start,
        filter_time_end_ns=filter_time_end)

    if args.no_export:
        t0 = time.time()
        summary = sp.parse_summary()
        elapsed = time.time() - t0

        print(f"\n=== QST v4 File Summary ===")
        print(f"  OS Version:     {summary['os_version']}")
        print(f"  CPUs:           {summary['num_cpus']}")
        print(f"  Clock Freq:     {summary['clock_freq']:,} Hz")
        start_dt = datetime.fromtimestamp(
            summary['capture_start_ns'] / 1e9, tz=timezone.utc)
        end_dt = datetime.fromtimestamp(
            summary['capture_end_ns'] / 1e9, tz=timezone.utc)
        print(f"  Capture Start:  {start_dt}")
        print(f"  Capture End:    {end_dt}")
        dur_s = (summary['capture_end_ns'] - summary['capture_start_ns']) / 1e9
        print(f"  Duration:       {dur_s:.3f} s")
        print(f"  TraceBuffer:    {summary['tracebuf_size']} B, data@{summary['data_offset']}")
        print(f"  Valid Buffers:  {summary['total_buffers']}")
        print(f"  Total Events:   {summary['total_events']:,}")
        print(f"  Process Names:  {summary['process_names']}")
        print(f"  Thread Names:   {summary['thread_names']}")
        print(f"  Parse Time:     {elapsed:.2f} s")
    else:
        fmt = args.format
        output_path = args.output
        if not output_path:
            base, _ = os.path.splitext(args.input)
            ext = '.json' if fmt == 'json' else '.perfetto'
            output_path = f"{base}{ext}"

        print(f"Parsing and exporting: {args.input}")
        print(f"  Format: {fmt}")
        if args.lightweight:
            print(f"  Mode: lightweight (thread/CPU events only)")
        if filter_time_start or filter_time_end:
            ts = args.time_start or '(trace start)'
            te = args.time_end or '(trace end)'
            print(f"  Time filter: {ts} → {te}")
        if filter_pids:
            print(f"  PID filter: {sorted(filter_pids)}")
        if filter_tids:
            print(f"  TID filter: {sorted(filter_tids)}")

        t0 = time.time()
        stats = sp.parse_and_export(output_path, fmt=fmt)
        elapsed = time.time() - t0

        print(f"\n=== Export Statistics ===")
        print(f"  Thread State Slices:  {stats.get('thread_slices', 0):,}")
        print(f"  CPU Running Slices:   {stats.get('cpu_running_slices', 0):,}")
        print(f"  Interrupt Slices:     {stats.get('interrupt_slices', 0):,}")
        print(f"  Interrupt Instants:   {stats.get('interrupt_instants', 0):,}")
        print(f"  KerCall Instants:     {stats.get('kercall_instants', 0):,}")
        print(f"  Comm Instants:        {stats.get('comm_instants', 0):,}")
        print(f"  System Instants:      {stats.get('system_instants', 0):,}")
        sync_flows = stats.get('sync_flows', 0)
        ipc_msg = stats.get('ipc_msg_flows', 0)
        ipc_ack = stats.get('ipc_send_ack_flows', 0)
        ipc_reply = stats.get('ipc_reply_flows', 0)
        sig_flows = stats.get('signal_flows', 0)
        total_flows = sync_flows + ipc_msg + ipc_ack + ipc_reply + sig_flows
        if total_flows:
            print(f"  Flow Events:          {total_flows:,}  "
                  f"(sync:{sync_flows:,} msg:{ipc_msg:,} "
                  f"ack:{ipc_ack:,} reply:{ipc_reply:,} "
                  f"signal:{sig_flows:,})")
        print(f"  Total Events:         {stats.get('total_events_processed', 0):,}")
        print(f"  Total Buffers:        {stats.get('total_buffers', 0):,}")
        print(f"  File Size:            {stats.get('file_size_kb', 0):.1f} KB")
        print(f"  Time:                 {elapsed:.2f} s")
        print(f"\nOpen https://ui.perfetto.dev and load {output_path}")


if __name__ == '__main__':
    main()
