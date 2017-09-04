#!/usr/bin/env python

from numpy import genfromtxt
import matplotlib.pyplot as plt

def read_data():

    measurement_data = genfromtxt('measurement_data.csv', delimiter=',') # read GP data from csv
    measurement_data = measurement_data[1:,:] # strip first line to remove header text

    location = measurement_data[:,0]
    output = measurement_data[:,1]

    gp_data = genfromtxt('gp_data.csv', delimiter=',')
    gp_data = gp_data[1:,:]

    x_range = gp_data[:,0]
    mean = gp_data[:,1]
    std = gp_data[:,2]

    return location, output, x_range, mean, std

def update_plot(fig, axes, p1, p2, p3, p4):
    #print("update_plot() called")

    location, output, x_range, mean, std = read_data()
    #plt.clf()
    #plt.plot(location, output, 'r+')
    #plt.plot(x_range, mean, '-b')
    #plt.plot(x_range, mean+2*std, ':b')
    #plt.plot(x_range, mean-2*std, ':b')

    p1.set_data(location,output)
    p2.set_data(x_range,mean)
    p3.set_data(x_range,mean+2*std)
    p4.set_data(x_range,mean-2*std)
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
    p2, = plt.plot([],[], '-b')
    p3, = plt.plot([],[], ':b')
    p4, = plt.plot([],[], ':b')
    while True:
        try:
            update_plot(fig, axes, p1, p2, p3, p4)
        except:
            pass
        plt.pause(1.0)

if __name__ == "__main__":
    main()
