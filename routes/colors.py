import io
import typing

from PIL import Image
from pydantic import BaseModel, conint, Field
from fastapi import APIRouter, Body, UploadFile
from fastapi.responses import Response
from starlette.requests import Request

from .errors.errors import *
from .utils.session import get_image_url
from .src.colors import py_cffi_colors as colors_ext
from .utils.function_utils import model_checker, get_upload_file


# File based
class ReplaceBody(BaseModel):
    colors: typing.Annotated[list[list[int]] , list[list[conint(ge=0, lt=256)]]]
    animated: typing.Optional[bool] = None
    max_distance: typing.Annotated[float, Field(strict=True, gt=0, allow_inf_nan=False)] = 16.0


class MergeBody(BaseModel):
    source_url: typing.Optional[str] = None
    destination_url: typing.Optional[str] = None
    num_colors: typing.Annotated[int, Field(strict=True, gt=0, lt=256)] = 16
    animated: typing.Optional[bool] = None
    max_distance: typing.Annotated[float, Field(strict=True, gt=0, allow_inf_nan=False, default=16.0)] = 16.0


# url based
class ReplaceBodyURL(ReplaceBody):
    image_url: str


class MergeBodyURL(BaseModel):
    source_url: str
    destination_url: str
    num_colors: typing.Annotated[int, Field(strict=True, gt=0, lt=256, default=16)] = 16
    animated: typing.Optional[bool] = None
    max_distance: typing.Annotated[float, Field(strict=True, gt=0, allow_inf_nan=False, default=16.0)] = 16.0


router = APIRouter(
    prefix='/image',
    tags=['image']
)


@router.post('/replace_colors')
async def replace_colors(request: Request, args: ReplaceBodyURL = Body(...)):
    app = request.app
    image_url = args.image_url

    inputcolors = args.colors
    colors = []
    for color in inputcolors:
        if not all(isinstance(num, int) and 0 <= num < 256 for num in color):
            raise ZNeitizException(400, 'colors must be list of RGB colors.')
        colors.extend(color)
    if len(colors) % 3:
        raise ZNeitizException(400, 'colors must be a multiple of 3.')

    max_distance = args.max_distance
    animated = args.animated

    im_bytes = await get_image_url(app, image_url)
    with io.BytesIO(im_bytes) as file:
        with Image.open(file) as im:
            if animated is None:
                animated = getattr(im, 'n_frames', 1) > 1
            func = colors_ext.replace_gif_colors if animated else colors_ext.replace_single_colors
            image = await func(im, colors, max_dist=max_distance)

    return Response(image.read(), media_type=f"image/{'gif' if animated else 'png'}")


@router.post('/replace_colors/file')
async def replace_colors_file(
    request: Request,
    image: UploadFile,
    args: ReplaceBody = model_checker(ReplaceBody),
):

    inputcolors = args.colors
    colors = []
    for color in inputcolors:
        if not all(isinstance(num, int) and 0 <= num < 256 for num in color):
            raise ZNeitizException(400, 'colors must be list of RGB colors.')
        colors.extend(color)
    if len(colors) % 3:
        raise ZNeitizException(400, 'colors must be a multiple of 3.')

    max_distance = args.max_distance
    animated = args.animated

    im_bytes = await get_upload_file(image)
    await image.close()

    with io.BytesIO(im_bytes) as file:
        with Image.open(file) as im:
            if animated is None:
                animated = getattr(im, 'n_frames', 1) > 1
            func = colors_ext.replace_gif_colors if animated else colors_ext.replace_single_colors
            output = await func(im, colors, max_dist=max_distance)

    return Response(output.read(), media_type=f"image/{'gif' if animated else 'png'}")


@router.post('/merge_colors')
async def merge_colors(request: Request, args: MergeBodyURL = Body(...)):
    app = request.app
    destination = args.destination_url
    source = args.source_url

    max_distance = args.max_distance
    animated = args.animated

    im_bytes = await get_image_url(app, destination)
    with Image.open(io.BytesIO(await get_image_url(app, source))) as im:
        colors = await colors_ext.extract_colors(im, args.num_colors)

    with io.BytesIO(im_bytes) as file:
        with Image.open(file) as im:
            if animated is None:
                animated = getattr(im, 'n_frames', 1) > 1
            func = colors_ext.replace_gif_colors if animated else colors_ext.replace_single_colors
            image = await func(im, colors, max_dist=max_distance)

    return Response(image.read(), media_type=f"image/{'gif' if animated else 'png'}")


@router.post('/merge_colors/file')
async def merge_colors_file(
    request: Request,
    source_image: typing.Optional[UploadFile] = None,
    destination_image: typing.Optional[UploadFile] = None,
    args: MergeBody = model_checker(MergeBody),
):

    app = request.app

    max_distance = args.max_distance
    animated = args.animated

    if destination_image:
        destination_bytes = await get_upload_file(destination_image)
        await destination_image.close()
    elif args.destination_url:
        destination_bytes = await get_image_url(app, args.destination_url)
    else:
        raise ZNeitizException(message='"destination_image" or "destination_url" is required.')

    if source_image:
        source_bytes = await get_upload_file(source_image)
        await source_image.close()
    elif args.source_url:
        source_bytes = await get_image_url(app, args.source_url)
    else:
        raise ZNeitizException(message='"source_image" or "source_url" is required.')

    with Image.open(io.BytesIO(source_bytes)) as im:
        colors = await colors_ext.extract_colors(im, args.num_colors)

    with io.BytesIO(destination_bytes) as file:
        with Image.open(file) as im:
            if animated is None:
                animated = getattr(im, 'n_frames', 1) > 1
            func = colors_ext.replace_gif_colors if animated else colors_ext.replace_single_colors
            image = await func(im, colors, max_dist=max_distance)

    return Response(image.read(), media_type=f"image/{'gif' if animated else 'png'}")
