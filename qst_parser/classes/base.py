"""
QST Parser - 事件类处理器基类

定义所有事件类处理器的基础接口。
"""

from abc import ABC, abstractmethod
from typing import Any, Dict, List, Optional


class BaseClassHandler(ABC):
    """
    事件类处理器基类
    
    所有事件类别处理器（thread, process, kercall 等）的基类。
    定义了统一的处理接口。
    
    所有 handler 必须:
    1. 接受 timestamp_ns 作为必须参数
    2. 在内部完成时间戳赋值，而不是返回后外部赋值
    3. 存储所有解析出的事件到对应的列表中
    """
    
    def __init__(self, verbose: bool = False):
        """
        初始化处理器
        
        Args:
            verbose: 是否输出详细日志
        """
        self.verbose = verbose
        self._combine_buffer: Dict[str, dict] = {}
    
    @abstractmethod
    def handle_event(
        self, 
        raw: Any, 
        ext_event: int, 
        timestamp_ns: int,
        **kwargs
    ) -> Optional[Any]:
        """
        处理单个事件
        
        Args:
            raw: 原始事件数据
            ext_event: 外部事件类型
            timestamp_ns: 已计算的 UTC 时间戳（纳秒），必须参数
            **kwargs: 额外参数
        
        Returns:
            解析后的事件对象，如果不产生输出则返回 None
            
        Note:
            handler 必须在内部完成 timestamp_ns 赋值，而不是返回后由调用者赋值
        """
        pass
    
    @property
    @abstractmethod
    def class_name(self) -> str:
        """返回事件类名称"""
        pass
    
    def log(self, message: str) -> None:
        """输出日志消息"""
        if self.verbose:
            print(f"[{self.class_name}] {message}")
    
    def clear_buffer(self) -> None:
        """清空组合事件缓冲区"""
        self._combine_buffer.clear()
