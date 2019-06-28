#!/usr/bin/env ../../../dependencies/python/install/bin/python3
# -*- coding: utf-8 -*-
#
# This script reads a fiber file and extracts fibers at regular distances.
#
# usage: extract_fibers.py <input filename> <output filename> <n_fibers_x target>

import sys, os
import numpy as np
import struct
import stl
from stl import mesh
import datetime
import pickle
import time

input_filename = "fibers.bin"

output_filename = "{}.out.bin".format(input_filename)

n_fibers_x_extract = 1

if len(sys.argv) >= 3:
  input_filename = sys.argv[1]
  output_filename = sys.argv[2]

if len(sys.argv) >= 4:
  n_fibers_x_extract = (int)(sys.argv[3])
  
print("{} -> {}".format(input_filename, output_filename))

with open(input_filename, "rb") as infile:
  
  # parse header
  bytes_raw = infile.read(32)
  header_str = struct.unpack('32s', bytes_raw)[0]
  
  header_length_raw = infile.read(4)
  header_length = struct.unpack('i', header_length_raw)[0]
  #header_length = 32+8
  parameters = []
  for i in range(int(header_length/4) - 1):
    int_raw = infile.read(4)
    value = struct.unpack('i', int_raw)[0]
    parameters.append(value)
    
  n_fibers_total = parameters[0]
  n_points_whole_fiber = parameters[1]
  n_fibers_x = (int)(np.sqrt(parameters[0]))
  n_fibers_y = n_fibers_x
  
  print("extract {} x {} fibers from file with {} x {} fibers\n".format(n_fibers_x_extract,n_fibers_x_extract,n_fibers_x,n_fibers_x))

  print("header: {}".format(header_str))
  print("nFibersTotal:      {n_fibers} = {n_fibers_x} x {n_fibers_x}".format(n_fibers=parameters[0], n_fibers_x=n_fibers_x))
  print("nPointsWholeFiber: {}".format(parameters[1]))
  print("nBorderPointsXNew: {}".format(parameters[2]))
  print("nBorderPointsZNew: {}".format(parameters[3]))
  print("nFineGridFibers_:  {}".format(parameters[4]))
  print("nRanks:            {}".format(parameters[5]))
  print("nRanksZ:           {}".format(parameters[6]))
  print("nFibersPerRank:    {}".format(parameters[7]))
  print("date:              {:%d.%m.%Y %H:%M:%S}".format(datetime.datetime.fromtimestamp(parameters[8])))
  
  streamlines = []
  n_streamlines_valid = 0
  n_streamlines_invalid = 0
  
  # loop over fibers
  for streamline_no in range(n_fibers_total):
    streamline = []
    streamline_valid = True
    
    # loop over points of fiber
    for point_no in range(n_points_whole_fiber):
      point = []
      
      # parse point
      for i in range(3):
        double_raw = infile.read(8)
        value = struct.unpack('d', double_raw)[0]
        point.append(value)
        
      # check if point is valid
      if point[0] == 0.0 and point[1] == 0.0 and point[2] == 0.0:
        if streamline_valid:
          coordinate_x = streamline_no % n_fibers_x
          coordinate_y = (int)(streamline_no / n_fibers_x)
          print("Error: streamline {}, ({},{})/({},{}) is invalid ({}. point)".format(streamline_no, coordinate_x, coordinate_y, n_fibers_x, n_fibers_y, point_no))
          print("streamline so far: ",streamline[0:10])
        streamline_valid = False
      streamline.append(point)
      
    if streamline_valid:
      n_streamlines_valid += 1
    else:
      n_streamlines_invalid += 1
      streamline = []
    streamlines.append(streamline)
  
  print("n valid: {}, n invalid: {}".format(n_streamlines_valid, n_streamlines_invalid))
  
  # create output file
  with open(output_filename,"wb") as outfile:
    
    infile.seek(0)
    raw_data = infile.read(32+header_length)
    outfile.write(raw_data)
    
    # write number of fibers
    n_fibers = n_fibers_x_extract * n_fibers_x_extract
    outfile.seek(32+4)
    outfile.write(struct.pack('i', n_fibers))
    
    # write timestamp
    outfile.seek(32+9*4)
    outfile.write(struct.pack('i', (int)(time.time())))
    
    # write fiber
    stride = (int)(np.floor(n_fibers_x / n_fibers_x_extract))
    offset = (int)((n_fibers_x - (n_fibers_x_extract-1)*stride) / 2)
    print("stride: {}, offset: {}".format(stride, offset))
    
    for y in range(offset, n_fibers_x_extract*stride, stride):
      print("select fiber {}/{}".format(y, n_fibers_x))
      
      for x in range(offset, n_fibers_x_extract*stride, stride):
        fiber = streamlines[y*n_fibers_x + x]
        # loop over points of fiber
        for point_no in range(n_points_whole_fiber):
          point = fiber[point_no]
          
          # parse point
          for i in range(3):
            double_raw = struct.pack('d', point[i])
            outfile.write(double_raw)
            
    print("File {} written.".format(output_filename))
    