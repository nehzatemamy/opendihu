# Single CellML problem, Shorten
# 
# Simulates a single instance of the Shorten 2007 problem for 10s. It is stimulated at time 0.0. Plots values of Vm and gamma in out.png.
# Note, this uses a very fine timestep width of 1e-5 and explicit integration. This is only for debugging and learning, you
# can replace the ExplicitEuler by, e.g., Heun integration
    
import numpy as np
import matplotlib 
matplotlib.use('Agg')
import matplotlib.pyplot as plt


# callback function that can set parameters, i.e. stimulation current. This is used to stimulate the CellML problem
def set_specific_parameters(n_nodes_global, time_step_no, current_time, parameters, additional_argument):
  
  # do not stimulate for t > 1
  if current_time > 0.1:
    return
  
  # determine nodes to stimulate (center node)  (this is not relevant here, as we only have 1 node which has no. 0)
  # but this code is included here if sb. wants to copy it to a different scenario where we have multiple nodes
  center_node = int(n_nodes_global / 2)
  nodes_to_stimulate_global = [center_node]
  
  # add 10 neighbours to the left and right (not really here, as we have only 1 node)
  for k in range(10):
    if center_node-k >= 0:
      nodes_to_stimulate_global.insert(0, center_node-k)
    if center_node+k <= n_nodes_global-1:
      nodes_to_stimulate_global.append(center_node+k)
  
  # set stimulation value of 40, set parameter no. 0 of node 0. (there is only one node here)
  stimulation_current = 40.
  
  for node_no_global in nodes_to_stimulate_global:
    parameters[(node_no_global, 0, 0)] = stimulation_current   # key: ([x,y,z], nodalDofIndex, parameterNo)

#fig = plt.figure(1)
#plt.ion()

xdata = []
gamma_data = []
vm_data = []

# callback function that receives the whole result values and produces plots while the simulation is running
def handle_result(nInstances, timeStepNo, currentTime, states, intermediates, additional_argument):
  #print "time step {}, t={}, nEntries: {}, dim: {}, data: {}".format(timeStepNo, currentTime, nEntries, dim, data)
  #print("time step {}, t={}".format(timeStepNo, currentTime))
  #print("intermediates: {}".format(intermediates))
    
  xdata.append(currentTime)
  gamma_data.append(intermediates[0])
  vm_data.append(states[0]*-0.58)
  
  #print "plot xdata: {} ydata: {}".format(str(xdata), str(ydata))
  
  # plot values of Vm and gamma
  plt.figure(1)
  plt.clf()
  ax1 = plt.gca()
  ax1.plot(xdata, vm_data, 'go-', label='$V_m$')
  plt.xlabel('t')
  plt.ylabel('$V_m$')
  ax2 = ax1.twinx()
  ax2.plot(xdata, gamma_data, 'ro-', label='$\gamma$')
  plt.ylabel('$\gamma$')    
  
  # ask matplotlib for the plotted objects and their labels
  lines, labels = ax1.get_legend_handles_labels()
  lines2, labels2 = ax2.get_legend_handles_labels()
  ax2.legend(lines + lines2, labels + labels2, loc=0)
  
  plt.savefig("out.png")
  #plt.draw()
    

config = {
  "ExplicitEuler" : {
    "timeStepWidth": 1e-5,            # dt of solver
    "endTime" : 10.0,                 # end simulation time of solver
    "initialValues": [],              # initial values (not used)
    "timeStepOutputInterval": 1e5,    # the interval when the current time will be printed in the console (e.g. 'Explicit Euler, timestep 100000/10000000, t=1')
    "inputMeshIsGlobal": True,        # for the mesh, not relevant here as we have no elements, only one node
    "nAdditionalFieldVariables": 0,   # only revelant if there are multiple nested solvers and they transfer additional data
    "dirichletBoundaryConditions": {},    # we do not set dirichlet BC
    
    "OutputWriter" : [
       #{"format": "Callback", "outputInterval": 1e4, "callback": callback},
      #{"format": "Paraview", "filename": "out", "binaryOutput": "false", "fixedFormat": False, "outputInterval": 1},
      {"format": "PythonFile", "filename": "out/result", "outputInterval": 1e5, "binary": True, "onlyNodalValues": True},
    ],

    "CellML" : {
      "nElements": 0,                     # information on the mesh to use, here we have no element, this means only 1 dof, i.e. 1 instance of the CellML problem
      "inputMeshIsGlobal": True,          # information on the mesh to use
      "sourceFilename": "shorten_ocallaghan_davidson_soboleva_2007.c",         # input C++ source file, can be either generated by OpenCMISS or OpenCOR from cellml model
      #"simdSourceFilename" : "simdcode.cpp",                                 # filename of the transformed C++ source file that gets generated from sourceFilename and is ready for multiple instances
      #"libraryFilename": "cellml_simd_lib.so",                               # filename of the compiled library, if not set a default filename will be used
      #"compilerFlags": "",        # additional flags to compile the cellml file, -O3 is already default
      "useGivenLibrary": False,                                               # if the library under "libraryFilename" should be used as-is, if False, it will be compiled and replaced
      #"statesInitialValues": [...],           # use this to set initial values for all states, if not set, the default values are parsed from the sourceFilename (which is usually a good idea)
                      
      "setSpecificParametersFunction":         set_specific_parameters,     # callback function that sets parameters like stimulation current
      "setSpecificParametersCallInterval":     1e3,                         # set_specific_parameters should be called every x ms
      "additionalArgument": None,                                           # additional last argument for set_specific_parameters
                      
      "handleResultFunction": handle_result,     # callback function that gets all current values and can do something with them, in this case, handle_result will output a plot to out.png
      "handleResultCallInterval": 1e4,          # interval in which handle_result will be called
      "handleResultFunctionAdditionalParameter": None,      # additional last argument for handle_result
      
      "stimulationLogFilename": "out/stimulation.log",          # filename of a log file that contains all stimulation events
      
      # parameters: I_Stim, l_hs. I_Stim (the stimulation current) is used as "intermediate" no. 32, l_hs (the relative half-sarcomere-length) is used as "constant" 65.
      "parametersUsedAsIntermediate": [32],    # list of intermediate value indices that will be set by parameters. Explicitely defined parameters that will be copied to intermediates, this vector contains the indices of the algebraic array.
      "parametersUsedAsConstant":     [65],    # list of constant value indices that will be set by parameters.
      "parametersInitialValues": [1200.0, 1.0],      # initial values for all parameters: I_Stim, l_hs
      "statesForTransfer": [],                 # in case of coupled solvers the states to transfer to the other solver, here we have no coupled solvers therefore this is not relevant, state 0 = Vm, rate 28 = gamma
      "intermediatesForTransfer": [],          # in case of coupled solvers the intermediates to transfer to the other solver, here we have no coupled solvers therefore this is not relevant
    },
  },
}