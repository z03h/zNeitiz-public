
from pydantic import BaseModel
from fastapi import APIRouter, Body
from fastapi.responses import Response

from .errors.errors import *
from .src.runescape import _runescape


router = APIRouter(prefix='/image', tags=['image'])


class RunescapeInput(BaseModel):
    text: str


@router.post('/runescape')
async def runescape(args: RunescapeInput = Body(...)):
    text = args.text
    if len(text) > 100:
        raise ZNeitizException(400, 'Text length must be <= 100')

    file, type = await _runescape.runescape(text)

    if file is None:
        raise ZNeitizException(400, 'Invalid text input')

    return Response(
        file.read(),
        media_type=f'image/{type}'
    )
