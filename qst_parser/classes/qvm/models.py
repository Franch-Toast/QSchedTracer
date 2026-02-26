"""
QST Parser - QVM 类数据模型

QVM (Hypervisor) 事件数据结构。
"""

from dataclasses import dataclass
from ...models.base import BaseEvent


@dataclass
class QvmEvent(BaseEvent):
    """
    QVM 事件基类
    """
    event_type: int = 0         # QvmEventType
    event_type_name: str = ""


@dataclass
class QvmGuestEvent(BaseEvent):
    """
    Guest 进入/退出事件 (GUEST_ENTER / GUEST_EXIT)
    """
    event_type: str = ""  # enter/exit
    guest_id: int = 0


@dataclass
class QvmIntrEvent(BaseEvent):
    """
    中断事件 (RAISE_INTR / LOWER_INTR)
    """
    event_type: str = ""  # raise/lower
    intr_num: int = 0


@dataclass
class QvmTimerEvent(BaseEvent):
    """
    定时器事件 (TIMER_CREATE / TIMER_FIRE)
    """
    event_type: str = ""  # create/fire
    timer_id: int = 0
