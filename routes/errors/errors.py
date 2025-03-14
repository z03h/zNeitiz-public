from __future__ import annotations
from typing import Optional

__all__ = ('ZNeitizException', 'UnsupportedType', 'PayloadTooLarge', 'TooManyRequests')


class ZNeitizException(Exception):
    def __init__(self, status: int = 400, message: str = 'An error occurred.'):
        self.status = status
        self.message = message


class UnsupportedType(ZNeitizException):
    def __init__(self, message: Optional[str] = None):
        super().__init__(415, message or 'Unsupported filetype.')


class PayloadTooLarge(ZNeitizException):
    def __init__(self, message: Optional[str] = None):
        super().__init__(413, message or 'URL content too large.')


class TooManyRequests(ZNeitizException):
    def __init__(self, message: Optional[str] = None):
        super().__init__(429, message or 'Too many requests.')
