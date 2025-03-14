
import aiohttp

from ..errors import errors

# 10MiB filesize limit
MAX_FILESIZE = 10_485_760

# content type limits
ACCEPTED_CONTENT_TYPE = {'image/png', 'image/gif', 'image/jpeg', 'image/jpg', 'image/webp'}


def get_session(app) -> aiohttp.ClientSession:
    session = app.state.session
    if not session:
        headers = {'User-Agent': f'zNeitiz aiohttp/{aiohttp.__version__}'}
        session = aiohttp.ClientSession(headers=headers)
        app.state.session = session
    return session


async def close_sesssion(app):
    if app.state.session:
        await app.state.session.close()
    app.state.session = None


async def get_image_url(app, url: str) -> bytes:
    session = get_session(app)
    async with session.get(url) as resp:
        if resp.content_type not in ACCEPTED_CONTENT_TYPE:
            raise errors.UnsupportedType(f'{resp.content_type} not accepted.')
        if resp.content_length and resp.content_length > MAX_FILESIZE:
            raise errors.PayloadTooLarge(f'{resp.content_length} greater than max allowed filesize (10 MB).')
        if not resp.ok:
            raise errors.ZNeitizException(resp.status, f'Could not download URL: {resp.reason}')

        return await resp.read()
