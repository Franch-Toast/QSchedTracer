"""
QST Parser - Thread 类数据模型

线程事件的数据结构定义。
"""

from dataclasses import dataclass
from ...models.base import BaseEvent


@dataclass
class ThreadEvent(BaseEvent):
    """
    线程状态事件
    
    表示线程状态变化（RUNNING, READY, RECEIVE 等）。
    
    数据格式 (Fast mode):
        - data[0]: timestamp (cycles)
        - data[1]: pid
        - data[2]: tid
    
    数据格式 (Wide mode):
        COMBINE_BEGIN:
            - data[0]: timestamp (cycles)
            - data[1]: pid
            - data[2]: tid
        COMBINE_END (或 COMBINE_CONT + END):
            - priority: 线程优先级 (1-255, 数字越大优先级越高)
            - policy: 调度策略 (SCHED_FIFO=1, SCHED_RR=2, SCHED_OTHER=3, etc.)
            - partition_id: APS 分区 ID (仅 APS 调度器)
            - sched_flags: 调度标志 (仅 APS 调度器)
    """
    pid: int = 0
    tid: int = 0
    state: str = ""
    is_vthread: bool = False
    is_wide: bool = False
    
    # Wide mode 扩展字段
    priority: int = 0
    policy: int = 0
    partition_id: int = 0
    sched_flags: int = 0
    
    @property
    def policy_name(self) -> str:
        """获取调度策略名称"""
        policy_names = {
            0: "UNKNOWN",
            1: "SCHED_FIFO",
            2: "SCHED_RR",
            3: "SCHED_OTHER",
            4: "SCHED_SPORADIC",
        }
        return policy_names.get(self.policy, f"POLICY_{self.policy}")
