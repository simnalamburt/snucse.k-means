#!/usr/bin/env python

import array
import struct
import sys


import numpy
import matplotlib
matplotlib.use('AGG')

import matplotlib.pyplot as plt

DIM = 2



def split_axes(data):
    return data[0::2], data[1::2]



class DataPlotter:
    def __init__(self, centroid_file, data_file, output_file):
        self.n_centroids, self.centroids = self.read_data(centroid_file)        
        self.n_data, self.data = self.read_data(data_file)
        self.output_file = output_file
                
    def plot(self):
        plt.figure(figsize=(8, 8))
        self.set_color_dist()

        self.plot_data()
        self.plot_centroid()

        plt.savefig(self.output_file)

    def set_color_dist(self):
        self.cent_c = 'red'
        self.data_c = 'green'
        
    def plot_centroid(self):
        c_x, c_y = split_axes(self.centroids)
        plt.scatter(c_x, c_y, c=self.cent_c, s=50, marker='x')

    def plot_data(self):
        d_x, d_y = split_axes(self.data)
        plt.scatter(d_x, d_y, c=self.data_c, s=4, edgecolor='none', alpha=0.8)
        
    def read_data(self, file_name):
        with open(file_name, 'rb') as input_f:
            size = struct.unpack('I', input_f.read(struct.calcsize('I')))[0]
            data = array.array('f')
            data.fromfile(input_f, size * DIM)
        return size, data

    
class PartPlotter(DataPlotter):
    def __init__(self, centroid_file, data_file, part_file, output_file):
        DataPlotter.__init__(self, centroid_file, data_file, output_file)        
        self.part_file = part_file        
            
    
    def set_color_dist(self):
        with open(self.part_file, 'rb') as part_f:        
            part_size = struct.unpack('I', part_f.read(struct.calcsize('I')))[0]
            if part_size != self.n_data:
                print("Partition size dose not match data size")
                sys.exit()
            partition = array.array('I')
            partition.fromfile(part_f, self.n_data)

        self.cent_c = range(self.n_centroids)
        self.data_c = partition


    


if __name__ == '__main__':
    if len(sys.argv) < 5:
        print('{0} input <centroid file> <data file> <output image>'.format(sys.argv[0]))
        print('{0} result <centroid file> <data file> <partition result> <output image>'.format(sys.argv[0]))


    mode = sys.argv[1]    
    if mode == 'input':
        plotter = DataPlotter(sys.argv[2], sys.argv[3], sys.argv[4])
        plotter.plot()
    elif mode == 'result':
        plotter = PartPlotter(sys.argv[2], sys.argv[3], sys.argv[4], sys.argv[5])
        plotter.plot()

    

