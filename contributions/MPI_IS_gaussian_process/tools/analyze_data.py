#!/usr/bin/env python

import csv
import os
import matplotlib.pyplot as plt
import code
import scipy.signal
from numpy import *

def read_data_from_folder(data_folder):
    data = [] # initialize empty data structure

    pixel_scale = -1

    datasets = os.walk(data_folder).next()[1]

    learning_cutoff = 0 # time to ignore at the start to allow for learning

    for ds, d in enumerate(datasets):
        files = os.walk('./data/' + d).next()[2]
        print '===== DATASET: {} ====='.format(d)
        r = -1

        for f in files:
            csv_file = data_folder + '/' + d + '/' + f
            with open(csv_file, 'rb') as csvfile:

                data.append([])
                guider_started = False
                settling_started = False
                csvreader = csv.reader(csvfile, delimiter=',', quotechar='"')
                for row in csvreader:
                    if (len(row) == 1):
                        if (row[0].split(' ')[0] == 'Guiding'):
                            if (row[0].split(' ')[1] == 'Begins'):
                                r = r + 1
                                data[ds].append([])
                                data[ds][r] = dict()
                                data[ds][r]['Time'] = []
                                data[ds][r]['RARawDistance'] = []
                                data[ds][r]['DECRawDistance'] = []
                                data[ds][r]['RAGuideDistance'] = []
                                data[ds][r]['DECGuideDistance'] = []
                                data[ds][r]['SNR'] = []
                                guider_started = True

                            if (row[0].split(' ')[1] == 'Ends' and not r == -1):
                                time = array(data[ds][r]['Time'])
                                ra_dist = array(data[ds][r]['RARawDistance']) * data[ds][r]['PixelScale']
                                dec_dist = array(data[ds][r]['DECRawDistance']) * data[ds][r]['PixelScale']
                                ra_guide = array(data[ds][r]['RAGuideDistance']) * data[ds][r]['PixelScale']
                                dec_guide = array(data[ds][r]['DECGuideDistance']) * data[ds][r]['PixelScale']
                                snr = array(data[ds][r]['SNR'])

                                ra_acc = (ra_guide.cumsum() + ra_dist)
                                total_dist = sqrt(power(ra_dist,2) + power(dec_dist,2))

                                ra_rms = sqrt(mean(power(ra_dist[(time > learning_cutoff)],2)))
                                dec_rms = sqrt(mean(power(dec_dist[(time > learning_cutoff)],2)))
                                total_rms = sqrt(mean(power(total_dist[(time > learning_cutoff)],2)))

                                data[ds][r]['RARMS'] = ra_rms
                                data[ds][r]['DECRMS'] = dec_rms
                                data[ds][r]['TotalRMS'] = total_rms

                                if (len(time) > 0 and time[-1]/60 >= 10):
                                    print '{:<14} | RA RMS: {:1.3f} | {:<14} | Dec RMS: {:1.3f} | Total RMS: {:1.3f} | Duration:{: 4.0f} | RelPerf: {: 6.1f}'.format(data[ds][r]['RAAlgorithm'], ra_rms, data[ds][r]['DecAlgorithm'], dec_rms, total_rms, time[-1]/60, 100*(1-ra_rms/dec_rms))

                                guider_started = False

                    if (len(row) == 2):
                        if (row[1] == ' Settling started'):
                            settling_started = True
                        if (row[1] == ' Settling complete'):
                            settling_started = False

                    if (guider_started and 0 < len(row) < 18): # non-guiding lines
                        if (row[0][0:11] == 'Pixel scale'):
                            pixel_scale = float(row[0][14:18])
                            data[ds][r]['PixelScale'] = pixel_scale

                        if (row[0].split(' = ')[0] == 'X guide algorithm'):
                            data[ds][r]['RAAlgorithm'] = row[0].split(' = ')[1]

                        if (row[0].split(' = ')[0] == 'Y guide algorithm'):
                            data[ds][r]['DecAlgorithm'] = row[0].split(' = ')[1]

                    if (guider_started and not settling_started and len(row) == 18): # guiding lines
                        if(row[2] == 'Mount'): # not interested in DROPped frames
                            data[ds][r]['Time'].append(float(row[1]))
                            data[ds][r]['RARawDistance'].append(float(row[5]))
                            data[ds][r]['DECRawDistance'].append(float(row[6]))
                            data[ds][r]['RAGuideDistance'].append(float(row[7]))
                            data[ds][r]['DECGuideDistance'].append(float(row[8]))
                            data[ds][r]['SNR'].append(float(row[16]))
    return data

def sq_dist(a, b):
    return power((matrix(a).transpose() - matrix(b)),2)

def covSE(a, b, t, l):
    return t * exp(-sq_dist(a, b)/(2*l**2))

def cov(a, b):
    return covSE(a, b, 50, 100)

def main():
    data = read_data_from_folder('data')

    ds = 0
    r = 0

    time = array(data[ds][r]['Time'])
    ra_dist = array(data[ds][r]['RARawDistance']) * data[ds][r]['PixelScale']
    ra_guide = array(data[ds][r]['RAGuideDistance']) * data[ds][r]['PixelScale']
    ra_acc = (ra_guide.cumsum() + ra_dist)

    #x = matrix(linspace(0,2000,200))
    #X = matrix(time)
    #Y = matrix(ra_acc).T

    #m = cov(x,X) * linalg.solve(cov(X,X) + eye(X.shape[1]),Y)
    #v = cov(x,x) - cov(x,X) * linalg.solve(cov(X,X) + eye(X.shape[1]),cov(X,x))

    #code.interact(local=dict(globals(), **locals()))

    #plt.plot(time, ra_acc, 'r+')
    #plt.plot(x.T, m, '-b')
    #plt.plot(x.T, m+2*matrix(diag(v)).T, '-b')
    #plt.plot(x.T, m-2*matrix(diag(v)).T, '-b')

    for dataset in data:
        nr = 0
        maxtime = 0
        miny = 0
        maxy = 0
        minx = 0
        maxx = 0
        fig = False
        for run in dataset:
            if (len(run['Time']) > 0 and run['Time'][-1]/60 >= 10):
                nr = nr + 1
                if (maxtime < run['Time'][-1]):
                    maxtime = run['Time'][-1]
                ra_dist = array(run['RARawDistance']) * run['PixelScale']
                ra_guide = array(run['RAGuideDistance']) * run['PixelScale']
                #ra_dist = (ra_guide.cumsum() + ra_dist)
                time = array(run['Time'])
                if (miny > min(ra_dist)):
                    miny = min(ra_dist)
                if (maxy < max(ra_dist)):
                    maxy = max(ra_dist)
                if (minx > min(time)):
                    minx = min(time)
                if (maxx < max(time)):
                    maxx = max(time)
                fig = True
        if fig:
            plt.figure()
        r = 0
        for run in dataset:
            if (len(run['Time']) > 0 and run['Time'][-1]/60 >= 10):
                r = r + 1
                time = array(run['Time'])
                ra_dist = array(run['RARawDistance']) * run['PixelScale']
                ra_guide = array(run['RAGuideDistance']) * run['PixelScale']
                ra_acc = (ra_guide.cumsum() + ra_dist)

                h = scipy.signal.firwin(numtaps=10, cutoff=40, nyq=160)
                ra_filt = scipy.signal.lfilter(h, 1.0, ra_dist)

                ax = plt.subplot(nr, 1, r)
                plt.plot(time, ra_filt, 'r-')
                plt.plot(time, ra_dist, 'b+')
                #plt.plot(time, ra_acc, 'r-+')
                ax.set_ylim(miny, maxy)
                ax.set_xlim(minx, maxx)
                plt.title('{} | Dataset {} | Run {}'.format(run['RAAlgorithm'], data.index(dataset), dataset.index(run)))

    run = data[0][1]
    ##run = data[1][0]
    ##run = data[2][11]
    #run = data[3][7]

    time = array(run['Time'])
    ra_dist = array(run['RARawDistance'])
    dec_dist = array(run['DECRawDistance'])
    ra_guide = array(run['RAGuideDistance'])
    dec_guide = array(run['DECGuideDistance'])
    snr = array(run['SNR'])

    with open('dataset03.csv', 'wb') as csvfile:
        csvwriter = csv.writer(csvfile, delimiter=',', )
        csvfile.write('Time,RARawDistance,DECRawDistance,RAGuideDistance,DECGuideDistance,SNR\r\n')
        for i in range(len(time)):
            csvfile.write('{:0.3f},{:0.3f},{:0.3f},{:0.3f},{:0.3f},{:0.2f}\r\n'.format(time[i], ra_dist[i], dec_dist[i], ra_guide[i], dec_guide[i], snr[i]))

    plt.show()

if __name__ == "__main__":
    main()
