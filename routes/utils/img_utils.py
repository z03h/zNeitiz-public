from __future__ import annotations
from typing import TYPE_CHECKING, Literal

if TYPE_CHECKING:
    from PIL import Image

def _limit_size(
    im: Image.Image,
    *,
    max_size: int = 256,
    force: bool = False,
    upscale_sample: Literal[0, 1, 2, 3, 4, 5] = 3,
    downscale_sample: Literal[0, 1, 2, 3, 4, 5] = 3,
) -> Image.Image:
    longest = max(im.size)
    if force or longest > max_size:
        ratio = max_size/longest
        if ratio == 1:
            # don't bother since it's the correct size already
            return im
        im = im.resize(
            tuple(max(1, int(i*ratio)) for i in im.size),  # type: ignore  # should be tuple[int, int] but it doesn't know
            resample=downscale_sample if ratio < 1 else upscale_sample
        )
    return im
