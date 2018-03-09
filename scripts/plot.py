#!/usr/bin/env python
# -*- coding: utf-8 -*-

#
# Script to visualize python output files.
#

import sys
import numpy as np
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D
from matplotlib import animation
from matplotlib import cm
import csv
import collections
import copy
from sets import Set
import os
import time
import pickle

def get_values(data, field_variable_name, component_name):
  """
    extract the values of a single component of a field variable
    :param data: a single dict containing the data
    :param field_variable_name: the name of the field variable to consider
    :param component_name: the name of the component of the field_variable. This is often "0"
  """
  
  for field_variable in data['data']:
    if field_variable['name'] == field_variable_name:
      for components in field_variable['components']:
        if components['name'] == component_name:
          values = components['values']
          return values
  
def get_min_max(data, field_variable_name, component_name):
  """
    get the minimum and maximum of a field_variable and component 
    :param data: list of dicts, from multiple input files
    :param field_variable_name: the name of the field variable to consider
    :param component_name: the name of the component of the field_variable. This is often "0"
  """
  
  min_value = None
  max_value = None
  
  # find minimum and maximum solution values
  for item in data:
    for field_variable in item['data']:
      if field_variable['name'] == field_variable_name:
        for components in field_variable['components']:
          if components['name'] == component_name:
            values = components['values']
            item_min = min(values)
            item_max = max(values)
            
            if min_value == None or item_min < min_value:
              min_value = item_min
            if max_value == None or item_max > max_value:
              max_value = item_max
            break
        break
  return min_value, max_value

files = ""

if len(sys.argv) > 1:
  files = sys.argv[1:]
else:
  # get all input data in current directory
  ls = os.listdir(".")

  # sort files by number in file name
  files = sorted(ls)

# extract the files that are npy files
solution_condition = lambda filename: "solution.npy" in filename
solution_shaped_condition = lambda filename: "solution_shaped.npy" in filename
solution_py_condition = lambda filename: ".py" in filename

solution_files = list(np.extract(map(solution_condition, files), files))
solution_shaped_files = list(np.extract(map(solution_shaped_condition, files), files))
solution_py_files = list(np.extract(map(solution_py_condition, files), files))

print len(solution_py_files),"files:",solution_py_files

data = []

# load py files
for solution_py_file in solution_py_files:

  with open(solution_py_file,'rb') as f:
    try:
      dict_from_file = eval(f.read())
    except:
      try: 
        with open(solution_py_file,'rb') as f:
          dict_from_file = pickle.load(f)
      except:
        dict_from_file = {}
        print("could not parse file \"{}\"".format(solution_py_file))
        continue
    
    #print "file: {}, dict: {}".format(solution_py_file,dict_from_file)

    data.append(dict_from_file)

if len(data) == 0:
  print "no data found."
  sys.exit(0)

dimension = data[0]['dimension']

####################
# 1D
if dimension == 1:
  
  min_value, max_value = get_min_max(data, "solution", "0")
  min_x, max_x = get_min_max(data, "geometry", "x")
  
  print "value range: [{}, {}]".format(min_value, max_value)
  
  # prepare plot
  fig = plt.figure()

  margin = abs(max_value - min_value) * 0.1
  ax = plt.axes(xlim=(min_x, max_x), ylim=(min_value - margin, max_value + margin))
  line, = ax.plot([], [], 'o-', lw=2)
  text = plt.figtext(0.2,0.8,"timestep",size=20)
  ax.set_xlabel('X')
  ax.set_ylabel('Y')
  
  def init():
    line.set_data([], [])
    text.set_text("timestep {}".format(0))
    return line,

  def animate(i):
    # display data
    xdata = get_values(data[i], "geometry", "x")
    ydata = get_values(data[i], "solution", "0")
    line.set_data(xdata, ydata)
    
    # display timestep
    if 'timeStepNo' in data[i]:
      timestep = data[i]['timeStepNo']
    if 'currentTime' in data[i]:
      current_time = data[i]['currentTime']
      
    max_timestep = len(data)-1
      
    text.set_text("timestep {}/{}, t = {}".format(timestep, max_timestep, current_time))
    
    return line,
    
  interval = 5000.0 / len(data)
        
  # call the animator.  blit=True means only re-draw the parts that have changed.
  anim = animation.FuncAnimation(fig, animate, init_func=init,
             frames=len(data), interval=interval, blit=False)

  plt.show()
  
####################
# 2D
if dimension == 2:
  
  min_value, max_value = get_min_max(data, "solution", "0")
  min_x, max_x = get_min_max(data, "geometry", "x")
  min_y, max_y = get_min_max(data, "geometry", "y")
  
  print "value range: [{}, {}]".format(min_value, max_value)
  
  # prepare plot
  fig = plt.figure()

  margin = abs(max_value - min_value) * 0.1
  ax = fig.add_subplot(111, projection='3d', xlim=(min_x, max_x), ylim=(min_y, max_y), zlim=(min_value-margin, max_value+margin))
  
  surface = ax.plot_surface([], [], [], cmap=cm.coolwarm, linewidth=1,rstride=1,cstride=1)
  text = plt.figtext(0.2,0.8,"timestep",size=20)
  ax.set_xlabel('X')
  ax.set_ylabel('Y')
  ax.set_zlabel('Z')
  
  # create mesh
  if data[0]["meshType"] == "StructuredRegularFixed" or data[0]["meshType"] == "RegularFixed":
    
    print "basisfunction: [{}], basisOrder: [{}]".format(data[0]["basisFunction"], data[0]["basisOrder"])
    
    if data[0]["basisFunction"] == "Lagrange":
      nEntries = dimension * [0]
      for i in range(dimension):
        nEntries[i] = data[0]["basisOrder"] * data[0]["nElements"][i] + 1
    
    x_positions = get_values(data[0], "geometry", "x")
    y_positions = get_values(data[0], "geometry", "y")
    
    X = np.reshape(x_positions, [nEntries[1], nEntries[0]])
    Y = np.reshape(y_positions, [nEntries[1], nEntries[0]])
    
    #print "x_positions shape: {}".format(len(x_positions))
    
  elif data[0]["meshType"] == "StructuredDeformable":
    pass
  
  def animate(i):
    ax.clear()
    
    # display data
    solution_shaped = get_values(data[i], "solution", "0")
    Z = np.reshape(solution_shaped, nEntries)
    
    #print "x shape: {}, y shape: {}, z shape: {}".format(X.shape, Y.shape, Z.shape)
    
    plot = ax.plot_surface(X, Y, Z, cmap=cm.coolwarm, linewidth=1,rstride=1,cstride=1)
    ax.set_zlim(min_value-margin, max_value+margin)
    ax.set_xlabel('X')
    ax.set_ylabel('Y')
    ax.set_zlabel('Z')
    
    # display timestep
    if 'timeStepNo' in data[i]:
      timestep = data[i]['timeStepNo']
      max_timestep = data[-1]['timeStepNo']
    if 'currentTime' in data[i]:
      current_time = data[i]['currentTime']
      
    text.set_text("timestep {}/{}, t = {}".format(timestep, max_timestep, current_time))
    
    return plot,
    
  interval = 5000.0 / len(data)
        
  # call the animator.  blit=True means only re-draw the parts that have changed.
  anim = animation.FuncAnimation(fig, animate,
             frames=len(data), interval=interval, blit=False)

  anim.save("anim.mp4")
  plt.show()
  
sys.exit(0)