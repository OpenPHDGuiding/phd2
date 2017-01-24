#!/usr/bin/env python

import code
from time import sleep
from numpy import genfromtxt
import numpy as np
import matplotlib.pyplot as plt
import threading

def read_data():

    spectrum_data = genfromtxt('spectrum_data.csv', delimiter=',') # read GP data from csv
    spectrum_data = spectrum_data[1:,:] # strip first line to remove header text

    periods = spectrum_data[:,0]
    amplitudes = spectrum_data[:,1]

    return periods, amplitudes
    
def update_plot(fig, axes, p1):

    periods, amplitudes = read_data()

    p1.set_data(periods, amplitudes)
    axes.relim()
    axes.autoscale_view(True,True,True)
    fig.canvas.draw()

def main():
    print("main function started")
    plt.ion()
    fig = plt.figure()
    axes = fig.add_subplot(111)
    p1, = plt.plot([],[],'ob')

    while True:
        try:
            update_plot(fig, axes, p1)
        except:
            pass
        plt.pause(1.0)

if __name__ == "__main__":
    main()
