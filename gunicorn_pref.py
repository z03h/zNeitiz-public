from multiprocessing import cpu_count

# Socket Path
# bind = 'unix:/home/z03h/zneitiz/gunicorn.sock'

# Worker Options
#workers = cpu_count()
#workers = 1
worker_class = 'uvicorn.workers.UvicornWorker'

# Logging Options
loglevel = 'info'
# accesslog = '/home/z03h/zneitiz/access_log'
# errorlog =  '/home/z03h/zneitiz/error_log'
