"""
QST Parser - Process 类数据模型

进程事件的数据结构定义。
"""

from dataclasses import dataclass
from ...models.base import BaseEvent


@dataclass
class ProcessCreateEvent(BaseEvent):
    """
    进程创建事件 (PROCCREATE / PROCCREATE_NAME)
    
    Fast/Wide 格式:
        - data[0]: timestamp
        - data[1]: ppid (父进程 ID)
        - data[2]: pid
        - name: 进程名称（组合事件中）
    """
    ppid: int = 0
    pid: int = 0
    name: str = ""


@dataclass
class ProcessDestroyEvent(BaseEvent):
    """
    进程销毁事件 (PROCDESTROY / PROCDESTROY_NAME)
    
    Fast/Wide 格式:
        - data[0]: timestamp
        - data[1]: ppid
        - data[2]: pid
        - name: 进程名称/路径（组合事件中）
    """
    ppid: int = 0
    pid: int = 0
    name: str = ""


@dataclass
class ThreadNameEvent(BaseEvent):
    """
    线程命名事件 (PROCTHREAD_NAME)
    
    Fast/Wide 格式:
        - data[0]: timestamp
        - data[1]: pid
        - data[2]: tid
        - name: 线程名称（组合事件中）
    """
    pid: int = 0
    tid: int = 0
    name: str = ""
