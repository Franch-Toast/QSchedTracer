"""
QST Parser - Interrupt 类数据模型

中断事件的数据结构定义。

参考: kercall_table_Events.html

事件类型:
- INTENTER / INTENTER_64: 中断进入
- INTEXIT: 中断退出
- INT_HANDLER_ENTER / INT_HANDLER_ENTER_64: ISR handler 进入
- INT_HANDLER_EXIT: ISR handler 退出
"""

from dataclasses import dataclass
from typing import Optional
from ...models.base import BaseEvent


@dataclass
class InterruptBaseEvent(BaseEvent):
    """中断事件基类"""
    interrupt_number: int = 0    # 中断号
    is_64bit: bool = False       # 是否为 64 位模式
    is_wide: bool = False        # 是否为 Wide 模式


@dataclass
class InterruptEnterEvent(InterruptBaseEvent):
    """
    中断进入事件 (INTENTER / INTENTER_64)
    
    Fast mode:
        32-bit: interrupt_number, ip
        64-bit: interrupt_number, empty, ip (64) - 需要 Wide 模式才能完整获取
    
    Wide mode:
        32-bit: interrupt_number, ip
        64-bit: interrupt_number, empty, ip (64)
    """
    ip: int = 0                  # 中断发生时的指令指针


@dataclass
class InterruptExitEvent(InterruptBaseEvent):
    """
    中断退出事件 (INTEXIT)
    
    Fast/Wide mode:
        interrupt_number, kernel_flag
    
    kernel_flag 含义:
        - 0: 中断在用户态发生
        - 非0: 中断在内核态发生
    """
    kernel_flag: int = 0         # 内核标志


@dataclass
class InterruptHandlerEnterEvent(InterruptBaseEvent):
    """
    ISR Handler 进入事件 (INT_HANDLER_ENTER / INT_HANDLER_ENTER_64)
    
    Fast mode:
        32-bit: pid, interrupt_number, ip, area
        64-bit: pid, interrupt_number, ip (64), area (64)
    
    Wide mode:
        32-bit: pid, interrupt_number, ip, area
        64-bit: pid, interrupt_number, ip (64), area (64)
    """
    pid: int = 0                 # 进程 ID
    ip: int = 0                  # 指令指针
    area: int = 0                # 区域指针


@dataclass
class InterruptHandlerExitEvent(InterruptBaseEvent):
    """
    ISR Handler 退出事件 (INT_HANDLER_EXIT)
    
    Fast/Wide mode:
        interrupt_number, sigevent
    
    sigevent 含义:
        - 0: ISR 没有返回事件
        - 非0: ISR 返回了一个 sigevent 用于通知等待的线程
    """
    sigevent: int = 0            # 信号事件


# 兼容旧代码
@dataclass
class InterruptEvent(BaseEvent):
    """
    通用中断事件（兼容旧代码）
    
    建议使用更具体的事件类型。
    """
    event_type: str = ""         # enter/exit/handler_enter/handler_exit
    interrupt_number: int = 0    # 中断号
    ip: int = 0                  # 指令指针
    kernel_flag: int = 0         # 内核标志 (exit)
    pid: int = 0                 # 进程 ID (handler)
    area: int = 0                # 区域指针 (handler)
    sigevent: int = 0            # 信号事件 (handler_exit)
