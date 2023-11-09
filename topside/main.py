import asyncio
from websockets.sync.client import connect

def graph_data(message):
    # placeholder



def communicate():
    with connect("ws://localhost:8765") as websocket:
        user_input = input("> ")
        websocket.send(user_input)
        message = websocket.recv()
        print("< " + message)

        if user_input == "get_data":
            graph_data(message)


while True:
    communicate()
