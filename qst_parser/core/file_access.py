"""
Layer 0: File Access Abstraction

Provides mmap-based and seek+read-based access to QST files.
mmap is preferred for zero-copy access; seek+read is the fallback.
"""

import mmap
import struct
from typing import Optional


class FileAccess:
    """Abstract file access interface."""

    def read_at(self, offset: int, size: int) -> bytes:
        raise NotImplementedError

    def unpack_at(self, fmt: struct.Struct, offset: int):
        raise NotImplementedError

    def uint32_at(self, offset: int) -> int:
        raise NotImplementedError

    def size(self) -> int:
        raise NotImplementedError

    def close(self):
        raise NotImplementedError

    def __enter__(self):
        return self

    def __exit__(self, *args):
        self.close()


_U32 = struct.Struct('<I')


class MmapAccess(FileAccess):
    """Zero-copy mmap-based file access (recommended)."""

    def __init__(self, filepath: str):
        self._f = open(filepath, 'rb')
        self._mm = mmap.mmap(self._f.fileno(), 0, access=mmap.ACCESS_READ)
        self._size = self._mm.size()

    def read_at(self, offset: int, size: int) -> bytes:
        return self._mm[offset:offset + size]

    def unpack_at(self, fmt: struct.Struct, offset: int):
        return fmt.unpack_from(self._mm, offset)

    def uint32_at(self, offset: int) -> int:
        return _U32.unpack_from(self._mm, offset)[0]

    def size(self) -> int:
        return self._size

    def close(self):
        if self._mm:
            self._mm.close()
            self._mm = None
        if self._f:
            self._f.close()
            self._f = None


class SeekReadAccess(FileAccess):
    """Seek+read fallback for constrained environments."""

    def __init__(self, filepath: str):
        self._f = open(filepath, 'rb')
        self._f.seek(0, 2)
        self._size = self._f.tell()
        self._f.seek(0)

    def read_at(self, offset: int, size: int) -> bytes:
        self._f.seek(offset)
        return self._f.read(size)

    def unpack_at(self, fmt: struct.Struct, offset: int):
        return fmt.unpack(self.read_at(offset, fmt.size))

    def uint32_at(self, offset: int) -> int:
        return _U32.unpack(self.read_at(offset, 4))[0]

    def size(self) -> int:
        return self._size

    def close(self):
        if self._f:
            self._f.close()
            self._f = None


def open_qst(filepath: str, use_mmap: bool = True) -> FileAccess:
    """Open a QST file with the preferred access method."""
    if use_mmap:
        try:
            return MmapAccess(filepath)
        except (OSError, ValueError):
            pass
    return SeekReadAccess(filepath)
