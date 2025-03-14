import io
import re
import math

from collections.abc import Generator
from typing import Any, Optional, Literal, Callable, Union

from PIL import Image, ImageFont, ImageDraw

from ...utils.function_utils import in_executor

__all__ = ('runescape',)


@in_executor()
def runescape(text: str) -> tuple[Optional[io.BytesIO], Optional[Literal['png', 'gif']]]:
    text = text.replace('\n', ' ')
    f, format = draw_rs_text(text)
    return f, format


def rs_wave_position(
    times: int,
    *,
    index: int = 0,
    num_steps: int = 8,
    height: int = 8,
    **kwargs
) -> Generator[tuple[float, float], Any, Any]:
    step = 2 * math.pi/num_steps
    start_step = step * index
    positions = [math.sin(start_step + step * i) * height for i in range(num_steps)]
    for i in range(times):
        yield 0, positions[(index+i) % len(positions)]


def rs_wave2_position(
    times: int ,
    *,
    index: int = 0,
    num_steps: int = 8,
    height: int = 8,
    **kwargs
) -> Generator[tuple[float, float], Any, Any]:
    step = 2 * math.pi/num_steps
    start_step = step * index
    positions = [math.sin(start_step + step * i) * height for i in range(num_steps)]
    for i in range(times):
        yield positions[(index+i) % len(positions)], positions[(index+i) % len(positions)]


def rs_shake_position(
    times: int,
    *,
    index: int = 0,
    frames: int = 30,
    height: int = 20,
    num_steps: int = 8,
    **kwargs
) -> Generator[tuple[float, float], Any, Any]:
    step = 2 * math.pi/num_steps
    start_step = step * index
    adjusted_frames = frames * 0.9
    reduction = height * (adjusted_frames - index)/adjusted_frames if index < adjusted_frames else 0

    positions = [math.sin(start_step + step * i) for i in range(frames)]
    for i in range(times):
        yield 0, positions[(index+i) % len(positions)] * reduction


def rs_scroll_position(
    *,
    text_x_size: int,
    im_width: int,
    x_offset: int,
    index: int = 0,
    frames: int = 30,
    **kwargs
) -> tuple[float, float]:
    offset = 20
    distance = text_x_size + im_width + offset
    step = distance/(frames - 2)
    return im_width + offset - x_offset - (index * step), 0


def rs_slide_position(
    *,
    text_y_size: int,
    im_height: int,
    y_offset: int,
    index: int = 0,
    frames: int = 30,
    **kwargs
) -> tuple[float, float]:
    moving = (frames * 0.2)
    if index < (moving - 3):
        start = -(text_y_size + y_offset/2)
        distance = text_y_size + im_height + y_offset/2 + 10
        step = distance/(moving * 2)
        return 0, start + step * index
    elif index > (frames - moving):
        distance = text_y_size + im_height + y_offset/2 + 10
        step = distance/(moving * 2)
        return 0, step * (index - (frames - moving))
    else:
        return 0, 0


def rs_flash_color(type, *, index, step=30):
    colors = (
        ((255,0,0), (255,255,0)), # red yellow
        ((0,0,255), (0,255,255)), # blue cyan
        ((0,150,0), (0,255,0)), # dark_green green
    )
    index = index % step
    return colors[type-1][index >= step/2]


def rs_glow_color(type, *, index, num_frames):
    colors = (
        ((255, 40, 43), (241, 196, 15), (56, 254, 132), (52, 142, 249),), # rgbcolors
        ((255,0,0), (255,0,255), (0,0,255)), # red purple blue
        ((0,150,0), (255,255,255), (0,255,255)) # green white cyan
    )
    colors = colors[type-1]
    with Image.new('RGB', (len(colors), 1)) as im:
        for x, color in enumerate(colors):
            im.putpixel((x, 0), color)
        im = im.resize((num_frames * 4, 1)).resize((num_frames, 1))
        return im.getpixel((index, 1))


def rs_get_color(color, index, num_frames):
    if color in RS_STATIC_COLORS:
        return RS_STATIC_COLORS[color]

    if color.startswith('flash'):
        return rs_flash_color(int(color[-1]), index=index)
    elif color.startswith('glow'):
        return rs_glow_color(int(color[-1]), index=index, num_frames=num_frames)


def draw_rs_text(full_text: str) -> tuple[Optional[io.BytesIO], Optional[Literal['png', 'gif']]]:
    re_colors = '|'.join([*RS_STATIC_COLORS, *RS_ANIMATED_COLORS])
    re_effects = '|'.join(RS_MOVEMENT_LOOKUP)
    pattern = rf'(?:(?P<color>{re_colors}):\s*)?(?:(?P<effect>{re_effects}):)?\s*(?P<text>.+)'
    match = re.match(pattern, full_text, flags=re.I)
    if not match:
        _color = 'yellow'
        animation = None
        text = 'buying gf'
    else:
        _color = match.group('color') or 'yellow'
        _color = _color.lower()
        animation = match.group('effect') or None

        text = match.group('text')
        if not text:
            return None, None

    if animation:
        method = RS_MOVEMENT_LOOKUP.get(animation.lower())
    else:
        method = None

    animated = method or _color in RS_ANIMATED_COLORS
    num_frames = 90 if method in (rs_scroll_position, rs_slide_position) else 30

    font = ImageFont.truetype('routes/src/runescape/rs.ttf', size=24)

    clamp_y = True if method in (rs_slide_position, rs_scroll_position) or not animated else False
    clamp_x = method == rs_scroll_position

    with Image.new('RGB', (1,1)) as image:
        draw = ImageDraw.Draw(image)
        x1, y1, x2,y2 = draw.textbbox((0, 0), text, font)
        text_x_size = x2 - x1
        text_y_size = y2 - y1

    height = 20 if method == rs_shake_position else 8
    duration = 40 if method in (rs_wave_position, rs_wave2_position,) else 30

    im_width = int(text_x_size + (height * 3)) if not clamp_x else 200
    im_height = int(text_y_size * 1.5) if clamp_y or method in (rs_scroll_position, rs_slide_position) else int(text_y_size * 2 + (height * 1.5))

    image = Image.new('RGB', (im_width, im_height), (69,69,69))

    startx = int((im_width - text_x_size)/2)
    starty = int(im_height/2) - int(text_y_size/2)

    posx = startx
    posy = starty
    shake_wave: tuple[Callable[..., Generator[tuple[float, float]]], ...] = (rs_shake_position, rs_wave_position, rs_wave2_position)
    slide_scroll = (rs_slide_position, rs_scroll_position)

    final = io.BytesIO()

    if animated:
        text_len = len(text)
        frames = []
        num_steps = 30 if method == rs_wave2_position else 15
        for i in range(num_frames):
            frame = image.copy()
            draw = ImageDraw.Draw(frame)
            color = rs_get_color(_color, i, num_frames)
            if method in shake_wave:
                for char, (x_offset, y_offset) in zip(text, method(text_len, index=i, height=height, num_steps=num_steps)):  # type: ignore  # should only return generator of tuple[float, float]
                    draw.text(
                        (posx + x_offset, posy + y_offset),
                        char,
                        font=font,
                        fill=color
                    )
                    _, _, posx, _ = draw.textbbox((posx, posy), char, font=font)
                posx = startx
            elif method in slide_scroll:
                x_offset, y_offset = method(
                    index=i,
                    text_x_size=text_x_size,
                    text_y_size=text_y_size,
                    im_width=im_width,
                    im_height=im_height,
                    x_offset=startx,
                    y_offset=starty,
                    frames=num_frames,
                )
                draw.text(
                    (posx + x_offset, posy + y_offset),  # type: ignore  # offsets should be floats
                    text,
                    font=font,
                    fill=color,
                )
            else:
                draw.text(
                    (posx, posy),
                    text,
                    font=font,
                    fill=color,
                )
            frames.append(frame.quantize(256))

        frames[0].save(final, format='gif', save_all=True, append_images=frames[1:], loop=0, duration=duration, optimize=True)
    else:
        color = RS_STATIC_COLORS.get(_color)
        draw = ImageDraw.Draw(image)
        draw.text(
            (posx, posy),
            text,
            font=font,
            fill=color,
        )
        image.save(final, format='fpng', optimize=True)

    final.seek(0)
    return final, 'gif' if animated else 'png'


RS_STATIC_COLORS: dict[str, tuple[int, int, int]] = {
    'yellow': (255,255,0),
    'cyan': (0,255,255),
    'red': (255,0,0),
    'green': (0,255,0),
    'purple': (255,0,255),
    'white': (255,255,255)
}

RS_ANIMATED_COLORS: tuple[str, ...] = (
    'flash1',
    'flash2',
    'flash3',
    'glow1',
    'glow2',
    'glow3',
)

RS_MOVEMENT_LOOKUP: dict[
    str,
    Callable[
        ...,
        Union[
            tuple[float, float],
            Generator[tuple[float, float], Any, Any]
        ]
    ]
] = {
    'wave': rs_wave_position,
    'wave2': rs_wave2_position,
    'shake': rs_shake_position,
    'scroll': rs_scroll_position,
    'slide': rs_slide_position,
}
