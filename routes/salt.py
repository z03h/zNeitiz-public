import io
import typing
from enum import IntEnum

import numpy as np
from PIL import Image
from pydantic import BaseModel, Field
from fastapi import APIRouter, Body, UploadFile
from fastapi.responses import Response
from starlette.requests import Request

from .errors.errors import *
from .utils.session import get_image_url
from .utils.img_utils import _limit_size
from .src.salt import py_cffi_salt as salt_ext
from .utils.function_utils import in_executor, model_checker, get_upload_file

router = APIRouter(
    prefix='/image',
    tags=['image']
)

class ParticleType(IntEnum):
    salt = 0
    pepper = 1
    water = 2
    piss = 3

#typing.Annotated[int, Field(strict=True, gt=0, lt=256)] = 16

# file models
class ParticleOptions(BaseModel):
    particle_type: typing.Optional[ParticleType] = ParticleType.salt
    speed: typing.Annotated[typing.Optional[int], Field(strict=True, gt=0, le=10, default=2)] = 2
    amount: typing.Annotated[typing.Optional[int], Field(strict=True, gt=0, le=20, default=8)] = 8


class ExplodeOptions(BaseModel):
    percent: typing.Annotated[typing.Optional[int], Field(strict=True, gt=0, le=100, default=80)] = 80

# URL models
class ParticleBodyURL(BaseModel):
    image_url: str

class ExplodeOptionsURL(ParticleBodyURL, ExplodeOptions):
    pass

class ParticleOptionsURL(ParticleBodyURL, ParticleOptions):
    pass





@router.post('/particles')
async def particles(request: Request, args: ParticleOptionsURL = Body(...)):
    app = request.app
    image_url = args.image_url
    skip = args.speed or 2
    new_particles = args.amount or 8
    particle_type = args.particle_type.value if args.particle_type else 0

    if not (0 < skip <= 10):
        raise ZNeitizException(400, 'speed must be an integer between 1 and 10.')
    if not (0 < new_particles <= 20):
        raise ZNeitizException(400, 'amount must be an integer between 1 and 25.')

    im_bytes = await get_image_url(app, image_url)

    with io.BytesIO(im_bytes) as file:
        with Image.open(file) as im:
            image = await _particles(
                im,
                num_frames=120,
                new_particles=new_particles,
                skip=skip,
                particle_type=particle_type
            )

    return Response(image.read(), media_type=f'image/gif')


@router.post('/particles/file')
async def particles_file(
    request: Request,
    image: UploadFile,
    args: ParticleOptions = model_checker(ParticleOptions)
):
    skip = args.speed or 2
    new_particles = args.amount or 8
    particle_type = args.particle_type.value if args.particle_type else 0

    if not (0 < skip <= 10):
        raise ZNeitizException(400, 'speed must be an integer between 1 and 10.')
    if not (0 < new_particles <= 20):
        raise ZNeitizException(400, 'amount must be an integer between 1 and 20.')

    im_bytes = await get_upload_file(image)

    with io.BytesIO(im_bytes) as file:
        with Image.open(file) as im:
            output = await _particles(
                im,
                num_frames=120,
                new_particles=new_particles,
                skip=skip,
                particle_type=particle_type
            )

    return Response(output.read(), media_type=f'image/gif')


@in_executor()
def _particles(
    im: Image.Image,
    *,
    num_frames: int = 400,
    new_particles: int = 10,
    skip: int = 2,
    particle_type: int = 0
) -> io.BytesIO:
    w, h = im.size
    if w != 128:
        ratio = 128/w
        im = im.resize((128, max(1, int(h*ratio))))
    im = _limit_size(im, max_size=600)

    im = im.convert('RGBA')
    ref = np.zeros([20 + skip * new_particles, im.width, 4], dtype=np.uint8)
    image = np.array(im)
    base = np.vstack((ref, image))
    frames = salt_ext.draw_particles(
        base,
        frames=num_frames,
        new_particles=new_particles,
        skip=skip,
        particle_type=particle_type
    )
    b = io.BytesIO()
    im_frames = [Image.fromarray(f) for f in frames]
    if particle_type == 1:
        new_ims = [Image.new('RGB', im_frames[0].size, (255, 255, 255)) for _ in range(len(im_frames))]
        for im, frame in zip(new_ims, im_frames):
            im.paste(frame, mask=frame)
        im_frames = new_ims
    im_frames[0].save(b, format='gif', save_all=True, append_images=im_frames[1:], loop=0, dispose=2, duration=40)
    b.seek(0)
    return b


@router.post('/explode')
async def explode(request: Request, args: ExplodeOptionsURL = Body(...)):
    app = request.app
    image_url = args.image_url
    percent = args.percent or 80

    if not (0 < percent <= 100):
        raise ZNeitizException(400, 'percent must be an integer between 1 and 100')

    im_bytes = await get_image_url(app, image_url)

    with io.BytesIO(im_bytes) as file:
        with Image.open(file) as im:
            w, h = im.size
            if w != 80:
                ratio = 80/w
                im = im.resize((80, max(1, int(h*ratio))))
            im = im.convert('RGBA')
            arr = np.array(im)
            side = np.zeros([arr.shape[0], 20, 4], dtype=np.uint8)
            arr = np.hstack([side, arr, side])
            arr = np.vstack([np.zeros([40, arr.shape[1], 4], dtype=np.uint8), arr])
            im = Image.fromarray(arr)
            if not arr[..., 3].any():
                raise ZNeitizException(400, 'Cannot be a blank image')

            frames = await salt_ext.draw_debris(arr, percent=percent)

    b = io.BytesIO()
    im_frames = [Image.fromarray(f) for f in frames]
    duration = [500, *(30 for _ in range(len(im_frames)))]
    im.save(b, format='gif', save_all=True, append_images=im_frames, loop=0, dispose=2, duration=duration)
    b.seek(0)
    return Response(b.read(), media_type=f'image/gif')


@router.post('/explode/file')
async def explode_file(
    request: Request,
    image: UploadFile,
    args: ExplodeOptions = model_checker(ExplodeOptions)
):
    percent = args.percent or 80

    if not (0 < percent <= 100):
        raise ZNeitizException(400, 'percent must be an integer between 1 and 100')

    im_bytes = await get_upload_file(image)

    with io.BytesIO(im_bytes) as file:
        with Image.open(file) as im:
            w, h = im.size
            if w != 80:
                ratio = 80/w
                im = im.resize((80, max(1, int(h*ratio))))
            im = im.convert('RGBA')
            arr = np.array(im)
            side = np.zeros([arr.shape[0], 20, 4], dtype=np.uint8)
            arr = np.hstack([side, arr, side])
            arr = np.vstack([np.zeros([40, arr.shape[1], 4], dtype=np.uint8), arr])
            im = Image.fromarray(arr)
            if not arr[..., 3].any():
                raise ZNeitizException(400, 'Cannot be a blank image')

            frames = await salt_ext.draw_debris(arr, percent=percent)

    b = io.BytesIO()
    im_frames = [Image.fromarray(f) for f in frames]
    duration = [500, *(30 for _ in range(len(im_frames)))]
    im.save(b, format='gif', save_all=True, append_images=im_frames, loop=0, dispose=2, duration=duration)
    b.seek(0)
    return Response(b.read(), media_type=f'image/gif')


@router.post('/dust')
async def dust(request: Request, args: ParticleBodyURL = Body(...)):
    image_url = args.image_url

    im_bytes = await get_image_url(request.app, image_url)

    with io.BytesIO(im_bytes) as file:
        with Image.open(file) as im:
            w, h = im.size
            if w != 128:
                ratio = 128/w
                im = im.resize((128, max(1, int(h*ratio))))
            im = _limit_size(im, max_size=600)
            im = im.convert('RGBA')
            arr = np.array(im)
            if not arr[..., 3].any():
                raise ZNeitizException(400, 'Cannot be a blank image')
            frames = await salt_ext.draw_dust(arr)

    b = io.BytesIO()
    im_frames = [Image.fromarray(f) for f in frames]
    im_frames[0].save(b, format='gif', save_all=True, append_images=im_frames[1:], loop=0, dispose=2, duration=30)
    b.seek(0)
    return Response(b.read(), media_type=f'image/gif')


@router.post('/dust/file')
async def dust_file(request: Request, image: UploadFile):

    im_bytes = await get_upload_file(image)

    with io.BytesIO(im_bytes) as file:
        with Image.open(file) as im:
            w, h = im.size
            if w != 128:
                ratio = 128/w
                im = im.resize((128, max(1, int(h*ratio))))
            im = _limit_size(im, max_size=600)
            im = im.convert('RGBA')
            arr = np.array(im)
            if not arr[..., 3].any():
                raise ZNeitizException(400, 'Cannot be a blank image')
            frames = await salt_ext.draw_dust(arr)

    b = io.BytesIO()
    im_frames = [Image.fromarray(f) for f in frames]
    im_frames[0].save(b, format='gif', save_all=True, append_images=im_frames[1:], loop=0, dispose=2, duration=30)
    b.seek(0)
    return Response(b.read(), media_type=f'image/gif')


@router.post('/sand')
async def sand(request: Request, args: ParticleBodyURL = Body(...)):
    image_url = args.image_url

    im_bytes = await get_image_url(request.app, image_url)

    with io.BytesIO(im_bytes) as file:
        with Image.open(file) as im:
            w, h = im.size
            if w != 128:
                ratio = 128/w
                im = im.resize((128, max(1, int(h*ratio))))
            im = _limit_size(im, max_size=600)
            im = im.convert('RGBA')
            arr = np.array(im)
            if not arr[..., 3].any():
                raise ZNeitizException(400, 'Cannot be a blank image')
            frames = await salt_ext.draw_crumble(arr)

    b = io.BytesIO()
    im_frames = [Image.fromarray(f) for f in frames]
    im_frames[0].save(b, format='gif', save_all=True, append_images=im_frames[1:], loop=0, dispose=2, duration=30)
    b.seek(0)
    return Response(b.read(), media_type=f'image/gif')


@router.post('/sand/file')
async def sand_file(request: Request, image: UploadFile):

    im_bytes = await get_upload_file(image)

    with io.BytesIO(im_bytes) as file:
        with Image.open(file) as im:
            w, h = im.size
            if w != 128:
                ratio = 128/w
                im = im.resize((128, max(1, int(h*ratio))))
            im = _limit_size(im, max_size=600)
            im = im.convert('RGBA')
            arr = np.array(im)
            if not arr[..., 3].any():
                raise ZNeitizException(400, 'Cannot be a blank image')
            frames = await salt_ext.draw_crumble(arr)

    b = io.BytesIO()
    im_frames = [Image.fromarray(f) for f in frames]
    im_frames[0].save(b, format='gif', save_all=True, append_images=im_frames[1:], loop=0, dispose=2, duration=30)
    b.seek(0)
    return Response(b.read(), media_type=f'image/gif')
