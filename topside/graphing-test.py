import matplotlib.pyplot as plt
import numpy as np

# Converting to array of chars
array_string = "[1,2,3,4,5,6,7,8,9]"
array_string = array_string.replace('[', '')
array_string = array_string.replace(']', '')

y_axis = array_string.split(',')

# Converting to array of ints

x_axis = [] # declaring x_axis

for i in range(0, len(y_axis)):
    y_axis[i] = int(y_axis[i])
    x_axis.append(i * 5)

# making array index at 5 because first mesurement happens five seconds after float is turned on
x_axis.pop(0)
x_axis.append(x_axis[-1] + 5)

plt.plot(x_axis, y_axis)
plt.savefig("output.jpg")
