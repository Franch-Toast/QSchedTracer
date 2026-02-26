"""
QST Parser - 事件类型转换工具

处理内部事件类/事件号到外部格式的转换。
参考: sys/trace.h, samples_parser.html
"""

from typing import Tuple
from ..constants import (
    InternalClass, ExternalClass, IntEventType,
    _TRACE_MAX_KER_CALL_NUM, KERCALL_64,
)
from ..classes.thread.constants import MAX_TH_STATE_NUM


def internal_to_external(int_class: int, int_event: int) -> Tuple[int, int]:
    """
    将内部事件类/事件号转换为外部格式
    
    QNX trace 系统使用内部格式存储事件，需要转换为外部格式以便分类处理。
    
    内部类 (_TRACE_*_C 定义):
        0: EMPTY
        1: CONTROL
        2: KER_CALL
        3: INT
        4: PR_TH (包含 PROCESS/THREAD/VTHREAD)
        5: SYSTEM
        6: USER
        7: COMM
        8: QUIP
        9: SEC
        10: QVM
    
    转换规则:
        - KER_CALL 类 (关键！):
            - event 0-127: KERCALLENTER
            - event 128-255: KERCALLEXIT
            - event 256-383: KERCALLINT
            - 需要处理 64 位标志 (bit 9)
        - PR_TH 类:
            - event >= 2 * MAX_TH_STATE_NUM: PROCESS 事件
            - event >= MAX_TH_STATE_NUM: VTHREAD 事件
            - 其他: THREAD 事件
        - INT 类: 根据 int_event 判断是 ENTER/EXIT/HANDLER_*
        - 其他类: 直接映射
    
    参考: QNX SAT samples_parser.html 官方示例
    
    Args:
        int_class: 内部事件类
        int_event: 内部事件号
    
    Returns:
        (外部事件类, 外部事件号) 元组
        如果无法转换返回 (-1, -1)
    """
    if int_class == InternalClass.EMPTY:
        return ExternalClass.EMPTY, int_event
    
    elif int_class == InternalClass.CONTROL:
        return ExternalClass.CONTROL, int_event
    
    elif int_class == InternalClass.KER_CALL:
        # 内核调用事件 - 根据事件号范围区分 ENTER/EXIT/INT
        # 参考: samples_parser.html
        
        # 提取并清除 64 位标志
        is_64bit = (int_event & KERCALL_64) != 0
        base_event = int_event & ~KERCALL_64
        
        # 根据范围确定外部类
        if base_event < _TRACE_MAX_KER_CALL_NUM:
            # 0-127: KERCALLENTER
            ext_class = ExternalClass.KERCALLENTER
            ext_event = base_event
        elif base_event < 2 * _TRACE_MAX_KER_CALL_NUM:
            # 128-255: KERCALLEXIT
            ext_class = ExternalClass.KERCALLEXIT
            ext_event = base_event - _TRACE_MAX_KER_CALL_NUM
        elif base_event < 3 * _TRACE_MAX_KER_CALL_NUM:
            # 256-383: KERCALLINT
            ext_class = ExternalClass.KERCALLINT
            ext_event = base_event - 2 * _TRACE_MAX_KER_CALL_NUM
        else:
            # 未知事件
            return -1, -1
        
        # 恢复 64 位标志到外部事件号
        if is_64bit:
            ext_event |= KERCALL_64
        
        return ext_class, ext_event
    
    elif int_class == InternalClass.INT:
        # 中断事件 - 根据 int_event 判断子类型
        # 
        # 关键理解 (参考 sys/trace.h, samples_parser.html):
        # - int_event 编码的是事件类型 (ENTRY=1, EXIT=2, HANDLER_ENTRY=3, etc.)
        # - 实际的中断号存储在 data[1] 中，不在事件号中
        # - 64位模式通过 int_event=5 (ENTRY_64) 或 6 (HANDLER_ENTRY_64) 标识
        # - ext_event 在 samples_parser.html 中被设为 -1（不使用）
        #
        # 这里返回 int_event 以便调用者可以判断是否是 64 位模式
        if int_event == IntEventType.ENTRY:
            return ExternalClass.INTENTER, int_event  # 32-bit
        elif int_event == IntEventType.ENTRY_64:
            return ExternalClass.INTENTER, int_event  # 64-bit (通过 int_event=5 区分)
        elif int_event == IntEventType.EXIT:
            return ExternalClass.INTEXIT, int_event
        elif int_event == IntEventType.HANDLER_ENTRY:
            return ExternalClass.INT_HANDLER_ENTER, int_event  # 32-bit
        elif int_event == IntEventType.HANDLER_ENTRY_64:
            return ExternalClass.INT_HANDLER_ENTER, int_event  # 64-bit (通过 int_event=6 区分)
        elif int_event == IntEventType.HANDLER_EXIT:
            return ExternalClass.INT_HANDLER_EXIT, int_event
        else:
            # 未知中断事件类型
            return ExternalClass.INT, int_event
    
    elif int_class == InternalClass.PR_TH:
        # 进程/线程/虚拟线程 - 共用内部类
        # 参考 sys/trace.h:
        #   - event < MAX_TH_STATE_NUM: THREAD
        #   - MAX_TH_STATE_NUM <= event < 2*MAX_TH_STATE_NUM: VTHREAD
        #   - event >= 2*MAX_TH_STATE_NUM: PROCESS (event>>6 获取类型)
        if int_event >= (2 * MAX_TH_STATE_NUM):
            # PROCESS 事件
            # 事件类型编码: _TRACE_PR_TH_*_P = (type << 6)
            # 外部事件号: 1 << ((event >> 6) - 1)
            ext_class = ExternalClass.PROCESS
            ext_event = 1 << ((int_event >> 6) - 1)
            return ext_class, ext_event
        elif int_event >= MAX_TH_STATE_NUM:
            # VTHREAD 事件
            ext_class = ExternalClass.VTHREAD
            ext_event = int_event - MAX_TH_STATE_NUM
            return ext_class, ext_event
        else:
            # THREAD 事件
            ext_class = ExternalClass.THREAD
            ext_event = int_event
            return ext_class, ext_event
    
    elif int_class == InternalClass.SYSTEM:
        return ExternalClass.SYSTEM, int_event
    
    elif int_class == InternalClass.USER:
        return ExternalClass.USER, int_event
    
    elif int_class == InternalClass.COMM:
        return ExternalClass.COMM, int_event
    
    elif int_class == InternalClass.QUIP:
        return ExternalClass.QUIP, int_event
    
    elif int_class == InternalClass.SEC:
        return ExternalClass.SEC, int_event
    
    elif int_class == InternalClass.QVM:
        return ExternalClass.QVM, int_event
    
    # 未知类别
    return -1, -1


def get_internal_class_name(int_class: int) -> str:
    """
    获取内部事件类名称
    
    Args:
        int_class: 内部事件类
    
    Returns:
        事件类名称字符串
    """
    class_names = {
        InternalClass.EMPTY: "EMPTY",
        InternalClass.CONTROL: "CONTROL",
        InternalClass.KER_CALL: "KER_CALL",
        InternalClass.INT: "INT",
        InternalClass.PR_TH: "PR_TH",
        InternalClass.SYSTEM: "SYSTEM",
        InternalClass.USER: "USER",
        InternalClass.COMM: "COMM",
        InternalClass.QUIP: "QUIP",
        InternalClass.SEC: "SEC",
        InternalClass.QVM: "QVM",
    }
    return class_names.get(int_class, f"UNKNOWN_{int_class}")


def get_external_class_name(ext_class: int) -> str:
    """
    获取外部事件类名称
    
    Args:
        ext_class: 外部事件类
    
    Returns:
        事件类名称字符串
    """
    try:
        return ExternalClass(ext_class).name
    except ValueError:
        return f"UNKNOWN_{ext_class}"
