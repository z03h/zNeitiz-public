import io
import sys
import typing

from PIL import Image, ImageSequence

from .cffi_color_replace import ffi, lib
from ...utils.img_utils import _limit_size
from ...utils.function_utils import in_executor

SIZE = 40


def color_distance(color1, color2) -> float:
    return lib.color_distance(*[float(i) for i in color1], *[float(i) for i in color2])


def color_distance2000(color1, color2) -> float:
    return lib.color_distance2000(*[float(i) for i in color1], *[float(i) for i in color2])


def shift_color(ref, offset, rgb) -> tuple[int, int, int]:
    color = lib.offset_rgb2(*(float(i) for i in (*ref, *offset, *rgb)))
    return (color.r, color.g, color.b)


@in_executor()
def replace_gif_colors(
    image: Image.Image,
    replace_colors: list,
    max_dist: float = 12.0,
    mode: int = 2,
    *,
    newsize: int = 512
):
    max_dist = float(max_dist)
    if not replace_colors or len(replace_colors) % 3 != 0 or any(not 0<=x<=255 for x in replace_colors):
        raise ValueError('replace_colors should be tuple/list of int, values between 0-255')
    lc = len(replace_colors)//3

    other_colorsp = ffi.new("ToReplace*")
    cp = ffi.new("int *", 0)
    other_colorsp.current = cp
    other_colorsp.size = len(replace_colors)

    colorsp = ffi.new('char []', [_.to_bytes(1, sys.byteorder) for _ in replace_colors])
    other_colorsp.colors = colorsp

    other_colors = other_colorsp[0]

    ccolors = lib.create_ptr(SIZE, 1)

    colors_ptrp = ffi.new('ReplacedColors*')
    colors_ptrp.colors = ccolors
    colors_ptrp.current = 0
    colors_ptrp.size = 1

    colors_ptr = colors_ptrp[0]

    palette_len = 768
    max_dist = max_dist

    frames = []
    duration = []
    for im in ImageSequence.Iterator(image):
        raw_ret = (b'\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00'*64)
        ret = ffi.new('char []', raw_ret)

        duration.append(im.info.get('duration', 40))
        original_size = im.size
        im = im.resize((im.width * 2, im.height * 2))
        original_alpha = im.convert('RGBA').getchannel('A')
        try:
            im = im.convert('RGB')
            im = im.quantize(lc, method=mode, dither=Image.NONE)
        except Exception:
            mode = 2
            im = im.quantize(lc, method=mode, dither=Image.NONE)
        palette, mapping = sort_palette(im)

        input_colors = ffi.new('double []', [float(x) for x in palette])

        colors_ptr = lib.replace_colors(input_colors, palette_len, max_dist, ret, colors_ptr, other_colors)
        palette = [int.from_bytes(x, sys.byteorder) for x in ret]
        new_palette = [0] * 768
        for i, index in enumerate(mapping):
            new_palette[index*3:index*3+3] = palette[i*3:i*3+3]

        im.putpalette(new_palette)
        im.info.pop('transparency', None)
        im = im.convert('RGBA')
        im.putalpha(original_alpha)
        if newsize:
            im = _limit_size(im.resize(original_size), max_size=newsize)
        frames.append(im.quantize(256, dither=Image.NONE))
    lib.free_ptr(colors_ptr.colors)
    return frames_to_image(frames, duration)


@in_executor()
def replace_single_colors(
    im: Image.Image,
    replace_colors: list,
    max_dist: float = 12.0,
    mode: typing.Literal[0, 1, 2, 3] = 2,
    *,
    newsize: int=1024
):
    max_dist = float(max_dist)
    if not replace_colors or len(replace_colors) % 3 != 0 or any(not 0<=x<=255 for x in replace_colors):

        raise ValueError(f'replace_colors should be tuple/list of int, values between 0-255\n{type(replace_colors)}, {type(replace_colors[0])}\n{replace_colors}')
    lc = min(len(replace_colors)//3 + 1, 256)

    other_colorsp = ffi.new("ToReplace*")
    cp = ffi.new("int *", 0)
    other_colorsp.current = cp
    other_colorsp.size = len(replace_colors)

    colorsp = ffi.new('char []', [_.to_bytes(1, sys.byteorder) for _ in replace_colors])
    other_colorsp.colors = colorsp

    other_colors = other_colorsp[0]

    ccolors = lib.create_ptr(SIZE, 1)

    colors_ptrp = ffi.new('ReplacedColors*')
    colors_ptrp.colors = ccolors
    colors_ptrp.current = 0
    colors_ptrp.size = 1

    colors_ptr = colors_ptrp[0]

    palette_len = 768
    max_dist = max_dist

    raw_ret = (b'\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00'*64)
    ret = ffi.new('char []', raw_ret)

    original_size = im.size
    im = im.resize((im.width * 2, im.height * 2))
    original_alpha = im.convert('RGBA').getchannel('A')
    try:
        im = im.convert('RGB')
        im = im.quantize(lc, method=mode, dither=Image.NONE)
    except Exception:
        mode = 2
        im = im.quantize(lc, method=mode, dither=Image.NONE)
    palette, mapping = sort_palette(im)

    input_colors = ffi.new('double []', [float(x) for x in palette])

    colors_ptr = lib.replace_colors(input_colors, palette_len, max_dist, ret, colors_ptr, other_colors)

    palette = [int.from_bytes(x, sys.byteorder) for x in ret]
    new_palette = [0] * 768
    for i, index in enumerate(mapping):
        new_palette[index*3:index*3+3] = palette[i*3:i*3+3]

    im.putpalette(new_palette)
    im.info.pop('transparency', None)
    im = im.convert('RGBA')
    im.putalpha(original_alpha)
    if newsize:
        im = _limit_size(im.resize(original_size), max_size=newsize)
    lib.free_ptr(colors_ptr.colors)

    return frames_to_image(im)


@in_executor()
def extract_colors(image, num_colors):
    image = image.quantize(num_colors, dither=Image.NONE)
    colors, _ = sort_palette(image)
    i = iter(colors)
    ret_colors = []
    while True:
        try:
            rgb = next(i), next(i), next(i)
            if any(rgb):
                ret_colors.extend(rgb)
        except StopIteration:
            return ret_colors


def frames_to_image(frames, durations=None):
    ret = io.BytesIO()
    if isinstance(frames, Image.Image):
        frames.save(ret, format='fpng', optimize=True)
        frames.close()
    elif isinstance(frames, list):
        frames[0].save(
            ret,
            format='gif',
            save_all=True,
            append_images=frames[1:],
            dispose=2,
            loop=0,
            duration=durations,
            optimize=True
        )
        for frame in frames:
            frame.close()

    ret.seek(0)
    return ret


def sort_palette(im):
    ret = [0, 0, 0] * 256
    mapping = []
    palette = im.getpalette()
    top_colors = sorted(im.getcolors(), reverse=True)
    for i in range(len(top_colors)):
        color_index = top_colors[i][1]
        mapping.append(color_index)
        color = palette[color_index * 3:(color_index + 1) * 3]
        ret[i*3:(i+1)*3] = color
    return ret, mapping
