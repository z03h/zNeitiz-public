
import numpy as np

from .cffi_salt import ffi, lib
from ...utils.function_utils import in_executor


__all__ = ('draw_particles', 'draw_debris', 'draw_dust', 'draw_crumble')


def draw_particles(
    arr: np.ndarray,
    *,
    frames: int = 400,
    new_particles: int = 12,
    skip: int = 2,
    particle_type: int = 0
) -> np.ndarray:
    ref = arr.copy()
    ref.flags.writeable = True
    ret = np.array([arr.copy() for _ in range(frames)], dtype=np.uint8)
    ret.flags.writeable = True

    retp= ffi.cast("char *", ret.ctypes.data)
    refp = ffi.cast("char *", ref.ctypes.data)
    shape = ffi.new("unsigned int []", ref.shape)
    stride = ffi.new("unsigned int []", ref.strides)

    lib.c_particles(refp, shape, stride, retp, frames, new_particles, skip, particle_type)

    return ret


@in_executor()
def draw_debris(
    arr: np.ndarray,
    *,
    num_frames: int = 75,
    percent: int = 100
) -> np.ndarray:
    active_arr = np.zeros([*arr.shape[:2]], dtype=int)
    active_arr.flags.writeable = True

    ref = arr.copy()
    ref.flags.writeable = True

    active_ref = np.zeros(ref.shape, dtype=np.uint8)
    active_ref.flags.writeable = True

    ret = np.zeros([num_frames, *arr.shape], dtype=np.uint8)
    ret.flags.writeable = True

    activep = ffi.cast('int  *', active_arr.ctypes.data)
    active_refp = ffi.cast('char  *', active_ref.ctypes.data)
    refp = ffi.cast('char *', ref.ctypes.data)
    retp = ffi.cast('unsigned char *', ret.ctypes.data)

    shape = ffi.new("unsigned int []", ref.shape)
    stride = ffi.new("unsigned int []", ref.strides)

    lib.c_debris(
        activep,
        refp,
        active_refp,
        shape,
        stride,
        retp,
        num_frames,
        percent,
    )

    return ret


@in_executor()
def draw_dust(arr: np.ndarray) -> np.ndarray:
    og_shape = arr.shape

    side = np.zeros([og_shape[0], og_shape[1]//4, og_shape[2]], dtype=np.uint8)
    short_side = np.zeros([og_shape[0], og_shape[1]//10, og_shape[2]], dtype=np.uint8)
    arr = np.hstack([short_side, arr, side])

    top = np.zeros([og_shape[0]//5, arr.shape[1], og_shape[2]], dtype=np.uint8)
    arr = np.vstack([top, arr, top])

    arr.flags.writeable = True
    shape = arr.shape

    frames = int(shape[1]* .7 + 25)

    ret = np.zeros([frames, *shape], dtype=np.uint8)
    ret.flags.writeable = True

    arrp = ffi.cast('char *', arr.ctypes.data)
    retp = ffi.cast('char *', ret.ctypes.data)

    shape = ffi.new("unsigned int []", arr.shape)
    stride = ffi.new("unsigned int []", arr.strides)

    lib.c_dust(arrp, shape, stride, int(og_shape[0]*og_shape[1]), frames, retp)

    return ret


@in_executor()
def draw_crumble(arr: np.ndarray) -> np.ndarray:
    shape = arr.shape
    side = np.zeros([shape[0], shape[0]//3, shape[2]], dtype=np.uint8)
    arr = np.hstack([side, arr, side])
    shape = arr.shape
    bottom = np.zeros([shape[0]//4, shape[1], shape[2]], dtype=np.uint8)
    arr = np.vstack([arr, bottom])

    ref = arr.copy()
    ref.flags.writeable = True

    active_arr = np.zeros([*arr.shape[:2]], dtype=int)
    active_arr.flags.writeable = True

    active_ref = np.zeros(ref.shape, dtype=np.uint8)
    active_ref.flags.writeable = True

    num_frames = int(arr.shape[0]/2 + shape[1]/4)

    ret = np.zeros([num_frames, *arr.shape], dtype=np.uint8)
    ret.flags.writeable = True

    active_arrp = ffi.cast('int *', active_arr.ctypes.data)
    active_refp = ffi.cast('char *', active_ref.ctypes.data)
    refp = ffi.cast('char *', ref.ctypes.data)
    retp = ffi.cast('char *', ret.ctypes.data)

    shape = ffi.new("unsigned int []", ref.shape)
    stride = ffi.new("unsigned int []", ref.strides)

    lib.c_crumble(
        active_arrp,
        active_refp,
        refp,
        shape,
        stride,
        retp,
        num_frames,
    )
    return ret
