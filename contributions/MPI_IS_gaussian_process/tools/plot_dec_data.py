#!/usr/bin/env python

import code
from time import sleep
from numpy import genfromtxt
import numpy as np
import matplotlib.pyplot as plt
import threading

def read_data():

    measurement_data = genfromtxt('tm_data.csv', delimiter=',') # read GP data from csv
    measurement_data = measurement_data[1:,:] # strip first line to remove header text

    location = measurement_data[:,0]
    output = measurement_data[:,1]

    gp_data = genfromtxt('gp_data.csv', delimiter=',')
    gp_data = gp_data[1:,:]

    x_range = gp_data[:,0]
    mean = gp_data[:,1]
    std = gp_data[:,2]

    return location, output

def update_plot(fig, axes, p1):
    #print("update_plot() called")

    location, output = read_data()
    #plt.clf()
    #plt.plot(location, output, 'r+')
    #plt.plot(x_range, mean, '-b')
    #plt.plot(x_range, mean+2*std, ':b')
    #plt.plot(x_range, mean-2*std, ':b')

    p1.set_data(location,output)

    axes.relim()
    axes.autoscale_view(True,True,True)
    fig.canvas.draw()
    #plt.draw()




def main():
    print("main function started")
    plt.ion()
    fig = plt.figure()
    axes = fig.add_subplot(111)
    p1, = plt.plot([],[],'r+')

    while True:
        try:
            update_plot(fig, axes, p1)
        except:
            pass
        plt.pause(1.0)

if __name__ == "__main__":
    main()
