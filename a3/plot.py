import matplotlib.pyplot as plt
import os
import numpy as np

names = ["atomic_counter", "naive_counter", "naive_counter_plus", "scalable_counter"]

def writeData():
    threads = [1, 2, 4, 8, 16, 32]

    for numThreads in threads:
        os.system(f"./naive_counter {str(numThreads)}")
        os.system(f"./naive_counter_plus {str(numThreads)}")
        os.system(f"./atomic_counter {str(numThreads)}")
        os.system(f"./scalable_counter {str(numThreads)}")

def plot():
    plt.xlabel("Number of Threads")
    plt.ylabel("Running Time (ms)")

    with open("atomic_counter_data.txt", "r") as atomic_counter:
        buildPlot(plt, atomic_counter, 0)
    with open("naive_counter_data.txt", "r") as naive_counter:
        buildPlot(plt, naive_counter, 1)
    with open("naive_counter_plus_data.txt", "r") as naive_counter_plus:
        buildPlot(plt, naive_counter_plus, 2)
    with open("scalable_counter_data.txt", "r") as scalable_counter:
        buildPlot(plt, scalable_counter, 3)

    plt.legend()
    plt.savefig("./data.png")


def buildPlot(plt, file, index):
    x_axis = [1,2,4,8,16,32]
    y_axis = []

    for i in range(6):
        line = file.readline()
        y_axis.append(int(float(line.strip())))

    plt.plot(x_axis, y_axis, marker = '.', markersize = 10, label=names[index])

writeData()
plot()