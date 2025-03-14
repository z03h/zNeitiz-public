import pathlib

from cffi import FFI


def build():
    ffibuilder = FFI()

    parent = pathlib.Path(__file__).parent.resolve()

    # For every function that you want to have a python binding,
    # specify its declaration here
    with open(f'{parent}/c_particles.h') as f:
        ffibuilder.cdef(f.read())

    # Here go the sources, most likely only includes and additional functions if necessary
    ffibuilder.set_source("cffi_salt",
        """
        #include "c_particles.h"
        """, sources=["c_particles.c", "debris.c", "dust.c", "salt.c"])
    ffibuilder.compile(str(parent))
