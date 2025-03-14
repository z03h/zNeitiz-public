from __future__ import annotations
from typing import TYPE_CHECKING, TypeVar

import asyncio
import functools

from pydantic import ValidationError
from fastapi import Form, Depends, UploadFile

from ..errors.errors import ZNeitizException, UnsupportedType

if TYPE_CHECKING:
    from typing import Callable, Awaitable
    from pydantic import BaseModel

T = TypeVar('T')
BM = TypeVar('BM')


def in_executor(executor=None):

    def decorator(func: Callable[..., T]) -> Callable[..., Awaitable[T]]:

        @functools.wraps(func)
        def wrapper(*args, **kwargs) -> Awaitable[T]:
            partial = functools.partial(func, *args, **kwargs)
            return asyncio.get_running_loop().run_in_executor(executor, partial)
        wrapper.original = func  # type: ignore  # monkey

        return wrapper

    return decorator


def model_checker(model: type[BaseModel]):

    async def checker(data: str = Form(...)) -> BaseModel:
        try:
            model_data = model.model_validate(data)
        except ValidationError:
            raise ZNeitizException(message='Could not parse form data')

        return model_data

    return Depends(checker)


def _get_mime_type_for_image(data: bytes):
    if data.startswith(b'\x89\x50\x4E\x47\x0D\x0A\x1A\x0A'):
        return 'image/png'
    elif data[0:3] == b'\xff\xd8\xff' or data[6:10] in (b'JFIF', b'Exif'):
        return 'image/jpeg'
    elif data.startswith((b'\x47\x49\x46\x38\x37\x61', b'\x47\x49\x46\x38\x39\x61')):
        return 'image/gif'
    elif data.startswith(b'RIFF') and data[8:12] == b'WEBP':
        return 'image/webp'
    else:
        raise UnsupportedType()


async def get_upload_file(file: UploadFile) -> bytes:
    try:
        inital_bytes = await file.read(50)
    except Exception:
        raise UnsupportedType
    else:
        _get_mime_type_for_image(inital_bytes)

    return inital_bytes + await file.read()
