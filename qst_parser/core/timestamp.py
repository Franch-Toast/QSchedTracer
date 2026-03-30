"""
Timestamp utilities for 32-bit cycle counter handling.

QNX traceevent stores only the low 32 bits of ClockCycles().
At 150MHz, the 32-bit counter wraps every ~28.6 seconds.
All cycle arithmetic must use signed interpretation for correctness.
"""

MAX_U32 = 0xFFFFFFFF
HALF_U32 = 0x7FFFFFFF


def signed_cycle_diff(a: int, b: int) -> int:
    """
    Compute (a - b) with correct 32-bit unsigned wraparound handling.
    Returns a signed integer: positive if a is "after" b, negative otherwise.
    Valid as long as the true difference is < ~14.3 seconds at 150MHz.
    """
    raw = (a - b) & MAX_U32
    if raw > HALF_U32:
        return raw - (MAX_U32 + 1)
    return raw


def cycle_gt(a: int, b: int) -> bool:
    """Is cycle a > cycle b? (wraparound-aware)"""
    d = signed_cycle_diff(a, b)
    return 0 < d <= HALF_U32


def cycles_to_ns(cycles: int, clock_freq: int) -> int:
    """Convert cycle count to nanoseconds."""
    if clock_freq == 0:
        return 0
    return cycles * 1_000_000_000 // clock_freq
