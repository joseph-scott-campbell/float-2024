import asyncio
from websockets.sync.client import connect
import matplotlib.pyplot as plt
import numpy as np

def graph_data(data_string):
    # converting data_string into y_axis: a list of chars
    data_string = data_string.replace('[', '')
    data_string = data_string.replace(']', '')
    y_axis = data_string.split(',')

    # converting y_axis into a list of ints and making x_axis
    x_axis = []
    for i in range(0, len(y_axis)):
        y_axis[i] = int(y_axis[i])
        x_axis.append(i * 5) # making array of 5 second increments

    # making array index at 5 because the first measurement happens 5
    # seconds after the float is turned on
    x_axis.pop(0)
    x_axis.append(x_axis[-1] + 5)

    # plotting the data and saving it to output.webp
    # if you don't like webp you can change it
    # they are great cause they are vector graphics that arn't SVGs
    plt.plot(x_axis, y_axis)
    plt.savefig("output.webp")


def communicate():
    with connect("ws://10.1.57.4") as websocket:
        user_input = input("> ")
        websocket.send(user_input)
        message = websocket.recv()
        print("< " + message)

        if user_input == "get_depth":
            graph_data(message)
        elif user_input == "profile":
            exit()
        elif user_input == "break":
            exit()


while True:
    communicate()
