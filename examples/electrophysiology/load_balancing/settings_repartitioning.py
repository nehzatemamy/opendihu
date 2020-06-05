# 2 fibers, biceps, example for load_balancing
#

end_time = 20.0     # end time for the simulation

import numpy as np
import pickle
import sys

# global parameters
PMax = 7.3              # maximum stress [N/cm^2]
Conductivity = 3.828    # sigma, conductivity [mS/cm]
Am = 500.0              # surface area to volume ratio [cm^-1]
Cm = 0.58               # membrane capacitance [uF/cm^2]
innervation_zone_width = 1.  # cm
innervation_zone_width = 0.  # cm
solver_type = "gmres"   # solver for the linear system

# timing parameters
stimulation_frequency = 10.0      # stimulations per ms
dt_1D = 1e-3                      # timestep width of diffusion
dt_0D = 3e-3                      # timestep width of ODEs
dt_3D = 3e-3                      # overall timestep width of splitting
output_timestep = 1e0             # timestep for output files

# input files
fiber_file = "../input/laplace3d_structured_linear"
fiber_distribution_file = "../input/MU_fibre_distribution_3780.txt"
firing_times_file = "../input/MU_firing_times_load_balancing.txt"
cellml_file = "../input/hodgkin_huxley_1952.c"
#cellml_file = "../input/shorten.cpp"        # <-- die Zeile einkommentieren, um Shorten zu verwenden

# get own rank no and number of ranks
rank_no = (int)(sys.argv[-2])
n_ranks = (int)(sys.argv[-1])

# set values for cellml model
if "shorten" in cellml_file:
  parameters_used_as_algebraic = [32]
  parameters_used_as_constant = [65]
  parameters_initial_values = [0.0, 1.0]
  nodal_stimulation_current = 1200.
  
elif "hodgkin_huxley" in cellml_file:
  parameters_used_as_algebraic = []
  parameters_used_as_constant = [2]
  parameters_initial_values = [0.0]
  nodal_stimulation_current = 40.

def get_motor_unit_no(fiber_no):
  """
  get the no. of the motor unit which fiber fiber_no is part of
  """
  return int(fiber_distribution[fiber_no % len(fiber_distribution)]-1)

def fiber_gets_stimulated(fiber_no, frequency, current_time):
  """
  determine if fiber fiber_no gets stimulated at simulation time current_time
  """

  # determine motor unit
  alpha = 1.0   # 0.8
  mu_no = (int)(get_motor_unit_no(fiber_no)*alpha)
  
  # determine if fiber fires now
  index = int(np.round(current_time * frequency))
  n_firing_times = np.size(firing_times,0)
  
  #if firing_times[index % n_firing_times, mu_no] == 1:
  #  print("{}: fiber {} is mu {}, t = {}, row: {}, stimulated: {} {}".format(rank_no, fiber_no, mu_no, current_time, (index % n_firing_times), firing_times[index % n_firing_times, mu_no], "true" if firing_times[index % n_firing_times, mu_no] == 1 else "false"))
  
  return firing_times[index % n_firing_times, mu_no] == 1
  
# callback function that can set states, i.e. prescribed values for stimulation
def set_specific_states(n_nodes_global, time_step_no, current_time, states, fiber_no):
  
  #print("call set_specific_states at time {}".format(current_time))
  
  # determine if fiber gets stimulated at the current time
  is_fiber_gets_stimulated = fiber_gets_stimulated(fiber_no, stimulation_frequency, current_time)

  if is_fiber_gets_stimulated:  
    # determine nodes to stimulate (center node, left and right neighbour)
    innervation_zone_width_n_nodes = innervation_zone_width*100  # 100 nodes per cm
    innervation_node_global = int(n_nodes_global / 2)  # + np.random.randint(-innervation_zone_width_n_nodes/2,innervation_zone_width_n_nodes/2+1)
    nodes_to_stimulate_global = [innervation_node_global]
    if innervation_node_global > 0:
      nodes_to_stimulate_global.insert(0, innervation_node_global-1)
    if innervation_node_global < n_nodes_global-1:
      nodes_to_stimulate_global.append(innervation_node_global+1)
    print("rank {}, t: {}, stimulate fiber {} at nodes {}".format(rank_no, current_time, fiber_no, nodes_to_stimulate_global))

    for node_no_global in nodes_to_stimulate_global:
      states[(node_no_global,0,0)] = 20.0   # key: ((x,y,z),nodal_dof_index,state_no)

# load fiber meshes, called streamlines
with open(fiber_file, "rb") as f:
  streamlines = pickle.load(f)

# assign fiber meshes to names streamline0, streamline1
if len(streamlines) < 2:
  print("Error: input file {} only contains {} fibers, 2 needed".format(fiber_file, len(streamlines)))
streamline0 = streamlines[0]
#streamline0 = streamlines[0][(int)(len(streamlines[0])/2)-5:(int)(len(streamlines[0])/2)+6]
streamline1 = streamlines[1]
    
# load MU distribution and firing times
fiber_distribution = np.genfromtxt(fiber_distribution_file, delimiter=" ")
firing_times = np.genfromtxt(firing_times_file)

# determine when the fibers will fire, this is only for debugging output and is not used by the config
if rank_no == 0:
  print("Debugging output about fiber firing: Taking input from file \"{}\"".format(firing_times_file))
  
  n_firing_times = np.size(firing_times,0)
  for fiber_no_index in range(2):
    first_stimulation = None
    for current_time in np.linspace(0,1./stimulation_frequency*n_firing_times,n_firing_times):
      if fiber_gets_stimulated(fiber_no_index, stimulation_frequency, current_time):
        first_stimulation = current_time
        break
  
    mu_no = get_motor_unit_no(fiber_no_index)
    print("   fiber {} is of MU {} and will be stimulated for the first time at {}".format(fiber_no_index, mu_no, first_stimulation))

# configure on which ranks fibers 0 and 1 will run
ranks = [
  [0,1,2,3],       # rank nos that will compute fiber 0
  #[2,3]        # rank nos that will compute fiber 1
]

config = {
  "scenarioName": "load_balancing",
  # the 1D meshes for each fiber
  "Meshes": {
    "MeshFiber0":
    {
      "nElements": len(streamline0)-1,
      "nodePositions": streamline0,
      "inputMeshIsGlobal": True,
      "setHermiteDerivatives": False
    },
    "MeshFiber1":
    {
      "nElements": len(streamline1)-1,
      "nodePositions": streamline1,
      "inputMeshIsGlobal": True,
      "setHermiteDerivatives": False
    }
  },
  # the linear solver for the diffusion problem
  "Solvers": {
    "implicitSolver": {
      "maxIterations": 1e4,
      "relativeTolerance": 1e-10,
      "absoluteTolerance": 1e-10,         # 1e-10 absolute tolerance of the residual    
      "solverType": solver_type,
      "preconditionerType": "none",
    }
  },
  # control class that configures multiple instances of the fiber model
  "MultipleInstances": {
    "nInstances": 1,      # number of fibers
    "instances": [        # settings for each fiber, `i` is the index of the fiber (0 or 1)
    {
      "ranks": ranks[i],
      
      "LoadBalancing": {
        "timeStepWidth": dt_3D,  # 1e-1
        "logTimeStepWidthAsKey": "dt_3D",
        "durationLogKey": "duration_total",
        "timeStepOutputInterval" : 1e2,
        "endTime": end_time,
      
        # config for strang splitting
        "StrangSplitting": {
          "timeStepWidth": dt_3D,  # 1e-1
          "logTimeStepWidthAsKey": "dt_3D",
          "durationLogKey": "duration_total",
          "timeStepOutputInterval" : 1,
          "endTime": end_time,
          "Term1": {      # CellML
            "HeunAdaptive" : {
              "timeStepWidth": dt_0D,  # 5e-5
              "tolerance": 1e7,
              "lowestMultiplier": 1000,
              "minTimeStepWidth": 1e-5,
              "timeStepAdaptOption": "regular",
              "logTimeStepWidthAsKey": "dt_0D",
              "durationLogKey": "duration_0D",
              "initialValues": [],
              "timeStepOutputInterval": 1e4,
              "inputMeshIsGlobal": True,
              "dirichletBoundaryConditions": {},
                
              "CellML" : {
                "modelFilename": cellml_file,                     # input C++ source file, can be either generated by OpenCMISS or OpenCOR from cellml model
                #"compilerFlags": "-fPIC -O3 -ftree-vectorize -shared",
                #"setParametersFunction": set_parameters,           # callback function that sets parameters like stimulation current
                #"setParametersCallInterval": int(1./stimulation_frequency/dt_0D),     # interval in which the set_parameters function will be called
                #"setParametersFunctionAdditionalParameter": i,
                
                "setSpecificStatesFunction": set_specific_states,    # callback function that sets states like Vm, activation can be implemented by using this method and directly setting Vm values, or by using setParameters/setSpecificParameters
                #"setSpecificStatesCallInterval": 2*int(1./stimulation_frequency/dt_0D),     # set_specific_states should be called stimulation_frequency times per ms, the factor 2 is needed because every Heun step includes two calls to rhs
                "setSpecificStatesCallInterval": 0,
                "setSpecificStatesCallFrequency": stimulation_frequency,     # set_specific_states should be called stimulation_frequency times per ms, the factor 2 is needed because every Heun step includes two calls to rhs
                "setSpecificStatesRepeatAfterFirstCall": 0.01,      # simulation time span for which the setSpecificStates callback will be called after a call was triggered
                "additionalArgument": i,
                
                "outputStateIndex": 0,                             # state 0 = Vm, rate 28 = gamma
                "parametersUsedAsAlgebraic": parameters_used_as_algebraic,  #[32],       # list of algebraic value indices, that will be set by parameters. Explicitely defined parameters that will be copied to algebraics, this vector contains the indices of the algebraic array. This is ignored if the input is generated from OpenCMISS generated c code.
                "parametersUsedAsConstant": parameters_used_as_constant,          #[65],           # list of constant value indices, that will be set by parameters. This is ignored if the input is generated from OpenCMISS generated c code.
                "parametersInitialValues": parameters_initial_values,            #[0.0, 1.0],      # initial values for the parameters: I_Stim, l_hs
                "meshName": "MeshFiber"+str(i),                    # name of the fiber mesh, i.e. either MeshFiber0 or MeshFiber1
                "prefactor": 1.0,
              },
            },
          },
          
          "Term2": {     # Diffusion
            "ImplicitEuler" : {
              "initialValues": [],
              #"numberTimeSteps": 1,
              "timeStepWidth": dt_1D,  # 1e-5
              "logTimeStepWidthAsKey": "dt_1D",
              "durationLogKey": "duration_1D",
              "timeStepOutputInterval": 1e4,
              "dirichletBoundaryConditions": {0: -75, -1: -75},      # set first and last value of fiber to -75
              "inputMeshIsGlobal": True,
              "solverName": "implicitSolver",
              "FiniteElementMethod" : {
                "maxIterations": 1e4,
                "relativeTolerance": 1e-10,
                "absoluteTolerance": 1e-10,         # 1e-10 absolute tolerance of the residual    
                "inputMeshIsGlobal": True,
                "meshName": "MeshFiber"+str(i),
                "prefactor": Conductivity/(Am*Cm),
                "solverName": "implicitSolver",
              },
              "OutputWriter" : [
                {"format": "Paraview", "outputInterval": int(1./dt_1D*output_timestep), "filename": "out/fiber_"+str(i), "binary": True, "fixedFormat": False, "combineFiles": False},
                #{"format": "Paraview", "outputInterval": 1, "filename": "out/fiber_"+str(i), "binary": True, "fixedFormat": False, "combineFiles": False},
                #{"format": "Paraview", "outputInterval": 1./dt_1D*output_timestep, "filename": "out/fiber_"+str(i)+"_txt", "binary": False, "fixedFormat": False},
                #{"format": "ExFile", "filename": "out/fiber_"+str(i), "outputInterval": 1./dt_1D*output_timestep, "sphereSize": "0.02*0.02*0.02"},
                #{"format": "PythonFile", "filename": "out/fiber_"+str(i), "outputInterval": int(1./dt_1D*output_timestep), "binary":True, "onlyNodalValues":True},
              ]
            },
          },
        }
        
      }
    }
    for i in range(1)
    ]
  }
}
