
try:
    import uvloop  # type: ignore  # optional uvloop if available
except Exception:
    pass
else:
    uvloop.install()

from contextlib import asynccontextmanager

from fastapi import FastAPI, Request
from fastapi.staticfiles import StaticFiles
from fastapi.responses import ORJSONResponse, HTMLResponse, FileResponse

from slowapi.extension import Limiter, _rate_limit_exceeded_handler
from slowapi.util import get_remote_address
from slowapi.middleware import SlowAPIMiddleware
from slowapi.errors import RateLimitExceeded

from uvicorn.middleware.proxy_headers import ProxyHeadersMiddleware

from routes.errors.errors import *
from routes import colors, salt, runescape

from plugins import FPngPlugin

FPngPlugin.plug()


@asynccontextmanager
async def lifespan(app: FastAPI):
    yield
    # clean up the session
    from routes.utils.session import close_sesssion
    await close_sesssion(app)


app = FastAPI(title='zNeitiz', summary='zNeitiz Image API', lifespan=lifespan)
app.mount("/static", StaticFiles(directory="static"), name="static")

app.state.session = app.state.pool = None

# slowapi limiter
limiter = Limiter(
    key_func=get_remote_address,
    default_limits=["10/minute"],
    headers_enabled=True
)
# TODO check on this later for errors?
app.state.limiter = limiter
app.add_exception_handler(RateLimitExceeded, _rate_limit_exceeded_handler)  # type: ignore  # from slowapi docs
app.add_middleware(SlowAPIMiddleware)
app.add_middleware(ProxyHeadersMiddleware)  # type: ignore  # i found this somewhere but I don't remember where

for module in (colors, salt, runescape):
    app.include_router(module.router)


# icon
@app.get('/favicon.ico', include_in_schema=False)
async def favicon():
    return FileResponse('static/favicon.ico')


@app.get('/', response_class=HTMLResponse)
@limiter.exempt
def home(r: Request):
    html_content = """
<html>
    <head>
        <meta property="og:title" content="zNeitiz" />
        <meta property="og:type" content="website" />
        <meta name="theme-color" content="#2f3136" />
        <meta property="og:site_name" content="zNeitiz Image API" />
        <meta content="/static/icon.png" property="og:image" />
        <title>\U0001f499 I love you \U0001f499</title>
    </head>
    <body>
        <h3>\U0001f499 I love you \U0001f499</h3>
        <p>Go to <a href='/docs'>/docs</a> for list of enpoints.<p>
    </body>
</html>
    """
    return HTMLResponse(content=html_content, status_code=200)


@app.get('/about')
@limiter.exempt
def about_api(r: Request):
    return HTMLResponse(
        content='''Go to <a href='/docs'>/docs</a> for list of enpoints.''',
        status_code=200
    )


#@app.get('/selftest')
@limiter.exempt
def self_test(r: Request):
    html = '''<html>
    <head>
        <meta property="og:title" content="zNeitiz" />
        <meta property="og:type" content="website" />
        <meta name="theme-color" content="#2f3136" />
        <meta property="og:site_name" content="zNeitiz Image API" />
        <meta content="/static/icon.png" property="og:image" />
        <title>\U0001f499 I love you \U0001f499</title>
    </head>

    <body>
        <h3>\U0001f499 I love you \U0001f499</h3>
        <p>Go to <a href='/docs'>/docs</a> for list of enpoints.<p>
        <br><br>
        {}
    </body>
</html>'''

    return HTMLResponse(content=html, status_code=200)


@app.exception_handler(ZNeitizException)
async def base_exception(request: Request, exc):
    return ORJSONResponse(status_code=exc.status, content={'message': exc.message})
