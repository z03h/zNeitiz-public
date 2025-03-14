import os
import cffi_make

os.system('pip install -U wheel setuptools')
os.system('pip install -Ur requirements.txt')

cffi_make.build()

# gunicorn main:app -c gunicorn_pref.py