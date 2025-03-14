
from PIL import Image, PngImagePlugin

__all__ = ('plug',)


def _load_save(func):
    try:
        Image.register_save('FPNG', func)
    except Exception:
        import traceback
        traceback.print_exc()


def plug():
    try:
        import fpnge  # type: ignore  # it's in try block

        def _fpng_save(im: Image.Image, file, filename=None, *args):  # type: ignore # yes it's potentially defined twice
            if im.mode == 'P' or im.mode == '1':
                return PngImagePlugin._save(im, file, filename, *args)  # type: ignore # fallback to regular png save for 1/P modes

            data = fpnge.fromPIL(im)
            file.write(data)

    except ModuleNotFoundError:
        try:
            import pyfpng  # type: ignore  # in try block
            import numpy as np

            def _fpng_save(im, file, filename=None, *args):
                if im.mode in ('RGBA', 'RGB'):
                    arr = np.array(im, dtype=np.uint8)
                    success, data = pyfpng.encode_image_to_memory(arr)
                    if not success:
                        # try original
                        return PngImagePlugin._save(im, file, filename, *args)  # type: ignore # fallback to regular on failure
                    file.write(data)
                    return

                return PngImagePlugin._save(im, file, filename, *args)  # type: ignore # fallback to regular png encoder if not RGB(A)

        except ModuleNotFoundError:
            print('Failed to load fpng/fpnge Image.save function')
        else:
            _load_save(_fpng_save)
    else:
        _load_save(_fpng_save)
