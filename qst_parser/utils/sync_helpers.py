"""
QST Parser - 同步原语辅助函数

提供 mutex/semaphore/condvar 字段解码功能。

QNX 同步原语的 count 和 owner 字段包含标志位和编码的进程/线程信息，
这些辅助函数用于提取实际值。
"""

from dataclasses import dataclass
from typing import Tuple, Optional


# ============================================================================
# Count 字段解析
# ============================================================================

# Mutex count 标志位含义 (高 8 位)
MUTEX_FLAG_INITIALIZED = 0x02  # Mutex 已初始化
MUTEX_FLAG_STATIC = 0x80       # 静态初始化的 mutex
MUTEX_FLAG_RECURSIVE = 0x04   # 递归 mutex (推测)

# Semaphore count 标志位含义
SEM_FLAG_INITIALIZED = 0x02   # Semaphore 已初始化


@dataclass
class CountInfo:
    """Count 字段解析结果"""
    raw_value: int          # 原始值
    flags: int              # 标志位 (高 8 位)
    actual_count: int       # 实际计数值 (低 24 位)
    is_initialized: bool    # 是否已初始化
    is_static: bool         # 是否静态初始化


def decode_count(count_raw: int) -> CountInfo:
    """
    解码 mutex/semaphore count 字段
    
    QNX 的 count 字段格式:
        - 高 8 位: 标志位
        - 低 24 位: 实际递归计数或信号量值
    
    Args:
        count_raw: 原始 count 值
    
    Returns:
        CountInfo 对象包含解析后的信息
    
    示例:
        >>> info = decode_count(0x02000001)
        >>> info.actual_count
        1
        >>> info.is_initialized
        True
    """
    if count_raw is None:
        count_raw = 0
    
    flags = (count_raw >> 24) & 0xFF
    actual_count = count_raw & 0x00FFFFFF
    
    return CountInfo(
        raw_value=count_raw,
        flags=flags,
        actual_count=actual_count,
        is_initialized=(flags & MUTEX_FLAG_INITIALIZED) != 0,
        is_static=(flags & MUTEX_FLAG_STATIC) != 0,
    )


def get_actual_count(count_raw: int) -> int:
    """
    快速获取实际计数值
    
    Args:
        count_raw: 原始 count 值
    
    Returns:
        实际计数值 (低 24 位)
    """
    if count_raw is None:
        return 0
    return count_raw & 0x00FFFFFF


# ============================================================================
# Owner 字段解析
# ============================================================================

@dataclass
class OwnerInfo:
    """Owner 字段解析结果"""
    raw_value: int      # 原始值
    pid: int            # 进程 ID (推测)
    tid: int            # 线程 ID (推测)
    has_owner: bool     # 是否有持有者
    description: str    # 描述字符串


def decode_owner(owner_raw: int) -> OwnerInfo:
    """
    解码 mutex/semaphore owner 字段
    
    QNX 的 owner 字段可能包含持有者的 pid/tid 信息。
    具体编码格式可能因 QNX 版本而异，以下是推测的格式:
    
    可能的格式:
        - 0: 无持有者
        - 其他: (flags << 24) | (pid_part << 8) | tid
        
    注意: 这是基于观察到的数据推测的格式，
          可能需要根据实际 QNX 版本调整。
    
    Args:
        owner_raw: 原始 owner 值
    
    Returns:
        OwnerInfo 对象包含解析后的信息
    
    示例:
        >>> info = decode_owner(0x50250027)
        >>> info.pid
        9472
        >>> info.tid
        39
    """
    if owner_raw is None or owner_raw == 0:
        return OwnerInfo(
            raw_value=0,
            pid=0,
            tid=0,
            has_owner=False,
            description="无持有者",
        )
    
    # 推测的格式: (flags << 24) | (pid_part << 8) | tid
    # 或: (some_id << 16) | tid
    
    # 方式 1: 按字节分析
    flags = (owner_raw >> 24) & 0xFF
    pid_part = (owner_raw >> 8) & 0xFFFF
    tid = owner_raw & 0xFF
    
    # 方式 2: 按 16 位分析
    high16 = (owner_raw >> 16) & 0xFFFF
    low16 = owner_raw & 0xFFFF
    
    # 选择最可能的解释
    # 如果 tid 在合理范围 (1-255) 且 pid_part 是合理的小数
    if tid > 0 and tid < 256 and pid_part > 0:
        return OwnerInfo(
            raw_value=owner_raw,
            pid=pid_part,
            tid=tid,
            has_owner=True,
            description=f"pid~{pid_part}, tid={tid}",
        )
    
    # 否则尝试其他解释
    return OwnerInfo(
        raw_value=owner_raw,
        pid=high16,
        tid=low16,
        has_owner=True,
        description=f"raw=0x{owner_raw:08x}",
    )


def get_owner_tid(owner_raw: int) -> int:
    """
    快速获取 owner 的线程 ID
    
    Args:
        owner_raw: 原始 owner 值
    
    Returns:
        线程 ID (低 8 位)，如果无 owner 返回 0
    """
    if owner_raw is None or owner_raw == 0:
        return 0
    return owner_raw & 0xFF


def get_owner_pid(owner_raw: int) -> int:
    """
    快速获取 owner 的进程 ID 部分
    
    Args:
        owner_raw: 原始 owner 值
    
    Returns:
        进程 ID 部分，如果无 owner 返回 0
    """
    if owner_raw is None or owner_raw == 0:
        return 0
    return (owner_raw >> 8) & 0xFFFF


# ============================================================================
# 便捷函数
# ============================================================================

def format_sync_ptr(sync_ptr: int) -> str:
    """
    格式化同步对象指针地址
    
    Args:
        sync_ptr: 64 位指针地址
    
    Returns:
        格式化的十六进制字符串
    """
    if sync_ptr is None:
        return "NULL"
    return f"0x{sync_ptr:016x}"


def is_kernel_address(address: int) -> bool:
    """
    检查地址是否为内核地址
    
    Args:
        address: 64 位地址
    
    Returns:
        True 如果是内核地址 (高位设置)
    """
    if address is None:
        return False
    # QNX 64-bit 内核地址通常有高位设置
    return (address >> 48) != 0


# ============================================================================
# 导出
# ============================================================================

__all__ = [
    # Count 相关
    'CountInfo',
    'decode_count',
    'get_actual_count',
    'MUTEX_FLAG_INITIALIZED',
    'MUTEX_FLAG_STATIC',
    
    # Owner 相关
    'OwnerInfo',
    'decode_owner',
    'get_owner_tid',
    'get_owner_pid',
    
    # 便捷函数
    'format_sync_ptr',
    'is_kernel_address',
]
