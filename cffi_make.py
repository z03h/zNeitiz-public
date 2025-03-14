
import os
import importlib
import traceback


# cffi
def build():
    path = 'routes/src'
    for folder in os.listdir(path):
        folder_path = f'{path}.{folder}.cffi_make'.replace("/", ".")
        try:
            cffi = importlib.import_module(folder_path)
            print(f'==== Building {folder.replace("/", ".")} ...')
            cffi.build()
        except (ModuleNotFoundError):
            print(f'==== Skipping {folder.replace("/", ".")}')
        except Exception as e:
            print(f'\tError building {folder.replace("/", ".")}')
            traceback.print_exc()
        print('\n')


if __name__ == '__main__':
    build()
