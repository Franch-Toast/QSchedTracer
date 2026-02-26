"""
QST Parser - Process 类常量定义

进程事件类型常量。
参考: kercall_table_Events.html
"""

from enum import IntEnum


class ProcessEventType(IntEnum):
    """
    进程事件类型
    
    外部事件号，通过位掩码编码。
    """
    PROCCREATE = 1         # 0x01 - 进程创建
    PROCCREATE_NAME = 2    # 0x02 - 进程创建（带名称）
    PROCDESTROY = 4        # 0x04 - 进程销毁
    PROCDESTROY_NAME = 8   # 0x08 - 进程销毁（带名称）
    PROCTHREAD_NAME = 16   # 0x10 - 线程命名
