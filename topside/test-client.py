import asyncio
from websockets.server import serve

async def echo(websocket):
    async for message in websocket:
        if message == "get_data":
            await websocket.send("[1,2,3,4,5,6]")
        else:
            await websocket.send(message)

async def main():
    async with serve(echo, "localhost", 8765):
        await asyncio.Future()  # run forever

asyncio.run(main())
