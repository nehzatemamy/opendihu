# Multiple 1D fibers (monodomain) with 3D static mooney rivlin with active contraction term, on biceps geometry.
# This example uses precice to couple the Monodomain eq. on fibers with the 3D contraction, both inside opendihu.
# You need to run both executables at the same time.

import sys, os
import timeit
import argparse
import importlib

# parse rank arguments
rank_no = (int)(sys.argv[-2])
n_ranks = (int)(sys.argv[-1])

# add variables subfolder to python path where the variables script is located
script_path = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, script_path)
sys.path.insert(0, os.path.join(script_path,'variables'))

import variables              # file variables.py, defined default values for all parameters, you can set the parameters there  
from create_partitioned_meshes_for_settings import *   # file create_partitioned_meshes_for_settings with helper functions about own subdomain

# if first argument contains "*.py", it is a custom variable definition file, load these values
if ".py" in sys.argv[0]:
  variables_path_and_filename = sys.argv[0]
  variables_path,variables_filename = os.path.split(variables_path_and_filename)  # get path and filename 
  sys.path.insert(0, os.path.join(script_path,variables_path))                    # add the directory of the variables file to python path
  variables_module,_ = os.path.splitext(variables_filename)                       # remove the ".py" extension to get the name of the module
  
  if rank_no == 0:
    print("Loading variables from \"{}\".".format(variables_path_and_filename))
    
  custom_variables = importlib.import_module(variables_module, package=variables_filename)    # import variables module
  variables.__dict__.update(custom_variables.__dict__)
  sys.argv = sys.argv[1:]     # remove first argument, which now has already been parsed
else:
  print("Warning: There is no variables file, e.g:\n ./biceps_contraction ../settings_biceps_contraction.py ramp.py\n")
  exit(0)


# -------------- begin user parameters ----------------

# timing parameters
# -----------------
variables.dt_0D = 1e-3                        # [ms] timestep width of ODEs
variables.dt_1D = 1.5e-3                      # [ms] timestep width of diffusion
variables.dt_splitting = 3e-3                 # [ms] overall timestep width of strang splitting
variables.dt_3D = 1e0                         # [ms] time step width of coupling, when 3D should be performed, also sampling time of monopolar EMG
# The values of dt_3D and end_time have to be also defined in "precice-config.xml" with the same value (the value is only significant in the precice-config.xml, the value here is used for output writer time intervals)
# <max-time value="100.0"/>           <!-- end time of the whole simulation -->
# <time-window-size value="1e0"/>   <!-- timestep width dt_3D -->
      

# quantities in mechanics unit system
rho = 10                    # [1e-4 kg/cm^3] density of the muscle (density of water)

# Mooney-Rivlin parameters [c1,c2,b,d] of c1*(Ibar1 - 3) + c2*(Ibar2 - 3) + b/d (λ - 1) - b*ln(λ)
# Heidlauf13: [6.352e-10 kPa, 3.627 kPa, 2.756e-5 kPa, 43.373] = [6.352e-11 N/cm^2, 3.627e-1 N/cm^2, 2.756e-6 N/cm^2, 43.373], pmax = 73 kPa = 7.3 N/cm^2
# Heidlauf16: [3.176e-10 N/cm^2, 1.813 N/cm^2, 1.075e-2 N/cm^2, 9.1733], pmax = 7.3 N/cm^2

c1 = 3.176e-10              # [N/cm^2]
c2 = 1.813                  # [N/cm^2]
b  = 1.075e-2               # [N/cm^2] anisotropy parameter
d  = 9.1733                 # [-] anisotropy parameter
variables.pmax = 7.3                  # [N/cm^2] maximum isometric active stress

# for debugging, b = 0 leads to normal Mooney-Rivlin
b = 0
variables.material_parameters = [c1, c2, b, d]   # material parameters

#variables.constant_body_force = (0,0,-9.81e-4)   # [cm/ms^2], gravity constant for the body force
variables.constant_body_force = (0,0,0)
#variables.bottom_traction = [0.0,0.0,-1e-1]        # [1 N]
#variables.bottom_traction = [0.0,0.0,-1e-3]        # [1 N]
variables.bottom_traction = [0.0,0.0,0.0]        # [1 N]
#variables.bottom_traction = [0.0,-1e-2,-5e-2]        # [N]


variables.fiber_file = "../../../../input/left_biceps_brachii_7x7fibers.bin"
#variables.fiber_file = "../../../../input/2x2fibers.bin"

# stride for sampling the 3D elements from the fiber data
# here any number is possible
variables.sampling_stride_x = 1
variables.sampling_stride_y = 1
variables.sampling_stride_z = 200

# enable paraview output
variables.paraview_output = True
variables.output_timestep = 1e0               # [ms] timestep for output files of fibers
variables.disable_firing_output = False

# -------------- end user parameters ----------------



# define command line arguments
parser = argparse.ArgumentParser(description='static_biceps_emg')
parser.add_argument('--scenario_name',                       help='The name to identify this run in the log.',   default=variables.scenario_name)
parser.add_argument('--n_subdomains', nargs=3,               help='Number of subdomains in x,y,z direction.',    type=int)
parser.add_argument('--n_subdomains_x', '-x',                help='Number of subdomains in x direction.',        type=int, default=variables.n_subdomains_x)
parser.add_argument('--n_subdomains_y', '-y',                help='Number of subdomains in y direction.',        type=int, default=variables.n_subdomains_y)
parser.add_argument('--n_subdomains_z', '-z',                help='Number of subdomains in z direction.',        type=int, default=variables.n_subdomains_z)
parser.add_argument('--diffusion_solver_type',               help='The solver for the diffusion.',               default=variables.diffusion_solver_type, choices=["gmres","cg","lu","gamg","richardson","chebyshev","cholesky","jacobi","sor","preonly"])
parser.add_argument('--diffusion_preconditioner_type',       help='The preconditioner for the diffusion.',       default=variables.diffusion_preconditioner_type, choices=["jacobi","sor","lu","ilu","gamg","none"])
parser.add_argument('--potential_flow_solver_type',          help='The solver for the potential flow (non-spd matrix).', default=variables.potential_flow_solver_type, choices=["gmres","cg","lu","gamg","richardson","chebyshev","cholesky","jacobi","sor","preonly"])
parser.add_argument('--potential_flow_preconditioner_type',  help='The preconditioner for the potential flow.',  default=variables.potential_flow_preconditioner_type, choices=["jacobi","sor","lu","ilu","gamg","none"])
parser.add_argument('--paraview_output',                     help='Enable the paraview output writer.',          default=variables.paraview_output, action='store_true')
parser.add_argument('--adios_output',                        help='Enable the MegaMol/ADIOS output writer.',          default=variables.adios_output, action='store_true')
parser.add_argument('--fiber_file',                          help='The filename of the file that contains the fiber data.', default=variables.fiber_file)
parser.add_argument('--fiber_distribution_file',             help='The filename of the file that contains the MU firing times.', default=variables.fiber_distribution_file)
parser.add_argument('--firing_times_file',                   help='The filename of the file that contains the cellml model.', default=variables.firing_times_file)
parser.add_argument('--end_time', '--tend', '-t',            help='The end simulation time.',                    type=float, default=variables.end_time)
parser.add_argument('--output_timestep',                     help='The timestep for writing outputs.',           type=float, default=variables.output_timestep)
parser.add_argument('--dt_0D',                               help='The timestep for the 0D model.',              type=float, default=variables.dt_0D)
parser.add_argument('--dt_1D',                               help='The timestep for the 1D model.',              type=float, default=variables.dt_1D)
parser.add_argument('--dt_splitting',                        help='The timestep for the splitting.',             type=float, default=variables.dt_splitting)
parser.add_argument('--dt_3D',                               help='The timestep for the 3D model, i.e. dynamic solid mechanics.', type=float, default=variables.dt_3D)
parser.add_argument('--disable_firing_output',               help='Disables the initial list of fiber firings.', default=variables.disable_firing_output, action='store_true')
parser.add_argument('--v',                                   help='Enable full verbosity in c++ code')
parser.add_argument('-v',                                    help='Enable verbosity level in c++ code', action="store_true")
parser.add_argument('-vmodule',                              help='Enable verbosity level for given file in c++ code')
parser.add_argument('-pause',                                help='Stop at parallel debugging barrier', action="store_true")

# parse command line arguments and assign values to variables module
args, other_args = parser.parse_known_args(args=sys.argv[:-2], namespace=variables)
if len(other_args) != 0 and rank_no == 0:
    print("Warning: These arguments were not parsed by the settings python file\n  " + "\n  ".join(other_args), file=sys.stderr)

# initialize some dependend variables
if variables.n_subdomains is not None:
  variables.n_subdomains_x = variables.n_subdomains[0]
  variables.n_subdomains_y = variables.n_subdomains[1]
  variables.n_subdomains_z = variables.n_subdomains[2]
  
# output information of run
if rank_no == 0:
  print("scenario_name: {},  n_subdomains: {} {} {},  n_ranks: {},  end_time: {}".format(variables.scenario_name, variables.n_subdomains_x, variables.n_subdomains_y, variables.n_subdomains_z, n_ranks, variables.end_time))
  print("dt_0D:           {:0.0e}, diffusion_solver_type:      {}".format(variables.dt_0D, variables.diffusion_solver_type))
  print("dt_1D:           {:0.0e}, potential_flow_solver_type: {}".format(variables.dt_1D, variables.potential_flow_solver_type))
  print("dt_splitting:    {:0.0e}, emg_solver_type:            {}, emg_initial_guess_nonzero: {}".format(variables.dt_splitting, variables.emg_solver_type, variables.emg_initial_guess_nonzero))
  print("dt_3D:           {:0.0e}, paraview_output: {}".format(variables.dt_3D, variables.paraview_output))
  print("output_timestep: {:0.0e}  stimulation_frequency: {} 1/ms = {} Hz".format(variables.output_timestep, variables.stimulation_frequency, variables.stimulation_frequency*1e3))
  print("fiber_file:              {}".format(variables.fiber_file))
  print("fat_mesh_file:           {}".format(variables.fat_mesh_file))
  print("cellml_file:             {}".format(variables.cellml_file))
  print("fiber_distribution_file: {}".format(variables.fiber_distribution_file))
  print("firing_times_file:       {}".format(variables.firing_times_file))
  print("********************************************************************************")
  
  print("prefactor: sigma_eff/(Am*Cm) = {} = {} / ({}*{})".format(variables.Conductivity/(variables.Am*variables.Cm), variables.Conductivity, variables.Am, variables.Cm))
  
  # start timer to measure duration of parsing of this script  
  t_start_script = timeit.default_timer()
    
# initialize all helper variables
from helper import *

variables.n_subdomains_xy = variables.n_subdomains_x * variables.n_subdomains_y
variables.n_fibers_total = variables.n_fibers_x * variables.n_fibers_y

# define the config dict
config = {
  "scenarioName":          variables.scenario_name,
  "logFormat":             "csv",
  "solverStructureDiagramFile":     "solver_structure.txt",     # output file of a diagram that shows data connection between solvers
  "mappingsBetweenMeshesLogFile":   "out/mappings_between_meshes.txt",
  "Meshes":                variables.meshes,
  "MappingsBetweenMeshes": variables.mappings_between_meshes,
  "Solvers": {
    "diffusionTermSolver": {# solver for the implicit timestepping scheme of the diffusion time step
      "maxIterations":      1e4,
      "relativeTolerance":  1e-10,
      "absoluteTolerance":  1e-10,         # 1e-10 absolute tolerance of the residual          
      "solverType":         variables.diffusion_solver_type,
      "preconditionerType": variables.diffusion_preconditioner_type,
      "dumpFilename":       "",   # "out/dump_"
      "dumpFormat":         "matlab",
    },
    "potentialFlowSolver": {# solver for the initial potential flow, that is needed to estimate fiber directions for the bidomain equation
      "relativeTolerance":  1e-10,
      "absoluteTolerance":  1e-10,         # 1e-10 absolute tolerance of the residual    
      "maxIterations":      1e4,
      "solverType":         variables.potential_flow_solver_type,
      "preconditionerType": variables.potential_flow_preconditioner_type,
      "dumpFilename":       "",
      "dumpFormat":         "matlab",
    },
    "mechanicsSolver": {   # solver for the dynamic mechanics problem
      "relativeTolerance":  1e-10,          # 1e-10 relative tolerance of the linear solver
      "absoluteTolerance":  1e-10,          # 1e-10 absolute tolerance of the residual of the linear solver       
      "solverType":         "preonly",      # type of the linear solver: cg groppcg pipecg pipecgrr cgne nash stcg gltr richardson chebyshev gmres tcqmr fcg pipefcg bcgs ibcgs fbcgs fbcgsr bcgsl cgs tfqmr cr pipecr lsqr preonly qcg bicg fgmres pipefgmres minres symmlq lgmres lcd gcr pipegcr pgmres dgmres tsirm cgls
      "preconditionerType": "lu",           # type of the preconditioner
      "maxIterations":       1e4,           # maximum number of iterations in the linear solver
      "snesMaxFunctionEvaluations": 1e8,    # maximum number of function iterations
      "snesMaxIterations":   10,            # maximum number of iterations in the nonlinear solver
      "snesRelativeTolerance": 1e-5,       # relative tolerance of the nonlinear solver
      "snesAbsoluteTolerance": 1e-5,        # absolute tolerance of the nonlinear solver
      "snesLineSearchType": "l2",        # type of linesearch, possible values: "bt" "nleqerr" "basic" "l2" "cp" "ncglinear"
      "snesRebuildJacobianFrequency": 5,
      "dumpFilename":        "",
      "dumpFormat":          "matlab",
    }
  },
  "PreciceAdapterVolumeCoupling": {
    "timeStepOutputInterval":   100,                        # interval in which to display current timestep and time in console
    "timestepWidth":            1,                          # coupling time step width, must match the value in the precice config
    "couplingEnabled":          True,                       # if the precice coupling is enabled, if not, it simply calls the nested solver, for debugging
    "preciceConfigFilename":    "precice_config.xml",       # the preCICE configuration file
    "preciceParticipantName":   "MuscleContraction",        # name of the own precice participant, has to match the name given in the precice xml config file
    "scalingFactor":            1,                          # a factor to scale the exchanged data, prior to communication
    "outputOnlyConvergedTimeSteps": True,                   # if the output writers should be called only after a time window of precice is complete, this means the timestep has converged
    
    "preciceData": [
      {
        "mode":                 "write",                    # mode is one of "read" or "write"
        "dataName":             "Geometry",                 # name of the vector or scalar to transfer, as given in the precice xml settings file
        "preciceMeshName":      "MuscleContractionMesh",    # name of the precice coupling mesh, as given in the precice xml settings file
        "opendihuMeshName":     None,                       # extra specification of the opendihu mesh that is used for the initialization of the precice mapping. If None or "", the mesh of the field variable is used.
        "slotName":             None,                       # name of the existing slot of the opendihu data connector to which this variable is associated to (only relevant if not isGeometryField)
        "isGeometryField":      True,                       # if this is the geometry field of the mesh
      },
      {
        "mode":                 "read",                     # mode is one of "read" or "write"
        "dataName":             "Gamma",                    # name of the vector or scalar to transfer, as given in the precice xml settings file
        "preciceMeshName":      "MuscleContractionMesh",    # name of the precice coupling mesh, as given in the precice xml settings file
        "opendihuMeshName":     None,                       # extra specification of the opendihu mesh that is used for the initialization of the precice mapping. If None or "", the mesh of the field variable is used.
        "slotName":             "gamma",                    # name of the existing slot of the opendihu data connector to which this variable is associated to (only relevant if not isGeometryField)
        "isGeometryField":      False,                      # if this is the geometry field of the mesh
      },
    ],
    
    "MuscleContractionSolver": {
      "mapGeometryToMeshes":          [],
      "numberTimeSteps":              1,                         # only use 1 timestep per interval
      "timeStepOutputInterval":       100,                       # do not output time steps
      "Pmax":                         variables.pmax,            # maximum PK2 active stress
      "enableForceLengthRelation":    True,                      # if the factor f_l(λ_f) modeling the force-length relation (as in Heidlauf2013) should be multiplied. Set to false if this relation is already considered in the CellML model.
      "lambdaDotScalingFactor":       1.0,                       # scaling factor for the output of the lambda dot slot, i.e. the contraction velocity. Use this to scale the unit-less quantity to, e.g., micrometers per millisecond for the subcellular model.
      "slotNames":                    ["lambda", "ldot", "gamma", "T"],   # names of the data connector slots
      "dynamic":                      False,                     # if the dynamic formulation with velocity or the quasi-static formulation is computed
      
      "OutputWriter" : [
        {"format": "Paraview", "outputInterval": int(1./variables.dt_3D*variables.output_timestep), "filename": "out/" + variables.scenario_name + "/mechanics", "binary": True, "fixedFormat": False, "onlyNodalValues":True, "combineFiles":True, "fileNumbering": "incremental"},
      ],
      
      "DynamicHyperelasticitySolver": {
        "timeStepWidth":              variables.dt_3D,           # time step width 
        "durationLogKey":             "nonlinear",               # key to find duration of this solver in the log file
        "timeStepOutputInterval":     1,                         # how often the current time step should be printed to console
        
        "materialParameters":         variables.material_parameters,  # material parameters of the Mooney-Rivlin material
        "density":                    variables.rho,             # density of the material
        "displacementsScalingFactor": 1.0,                       # scaling factor for displacements, only set to sth. other than 1 only to increase visual appearance for very small displacements
        "residualNormLogFilename":    "log_residual_norm.txt",   # log file where residual norm values of the nonlinear solver will be written
        "useAnalyticJacobian":        True,                      # whether to use the analytically computed jacobian matrix in the nonlinear solver (fast)
        "useNumericJacobian":         False,                     # whether to use the numerically computed jacobian matrix in the nonlinear solver (slow), only works with non-nested matrices, if both numeric and analytic are enable, it uses the analytic for the preconditioner and the numeric as normal jacobian
          
        "dumpDenseMatlabVariables":   False,                     # whether to have extra output of matlab vectors, x,r, jacobian matrix (very slow)
        # if useAnalyticJacobian,useNumericJacobian and dumpDenseMatlabVariables all all three true, the analytic and numeric jacobian matrices will get compared to see if there are programming errors for the analytic jacobian
        
        "loadFactors":                [],                        # no load factors, solve problem directly
        "loadFactorGiveUpThreshold":  4e-2,                      # a threshold for the load factor, when to abort the solve of the current time step. The load factors are adjusted automatically if the nonlinear solver diverged. If the progression between two subsequent load factors gets smaller than this value, the solution is aborted.
        "scaleInitialGuess":          False,                     # when load stepping is used, scale initial guess between load steps a and b by sqrt(a*b)/a. This potentially reduces the number of iterations per load step (but not always).
        "nNonlinearSolveCalls":       1,                         # how often the nonlinear solve should be repeated
    
        # mesh
        "inputMeshIsGlobal":          False,                     # the mesh is given locally
        "meshName":                   "3Dmesh_quadratic",        # name of the 3D mesh, it is defined under "Meshes" at the beginning of this config
        "fiberDirection":             None,                      # if fiberMeshNames is empty, directly set the constant fiber direction, in element coordinate system
        "fiberDirectionInElement":    [0,0,1],                   # if fiberMeshNames is empty, directly set the constant fiber direction, in element coordinate system
      
        # solver
        "solverName":                 "mechanicsSolver",         # name of the nonlinear solver configuration, it is defined under "Solvers" at the beginning of this config
        
        # boundary and initial conditions
        "dirichletBoundaryConditions": variables.elasticity_dirichlet_bc,   # the initial Dirichlet boundary conditions that define values for displacements u and velocity v
        "neumannBoundaryConditions":   variables.elasticity_neumann_bc,     # Neumann boundary conditions that define traction forces on surfaces of elements
        "updateDirichletBoundaryConditionsFunction": None,                  # function that updates the dirichlet BCs while the simulation is running
        "updateDirichletBoundaryConditionsFunctionCallInterval": 1,         # every which step the update function should be called, 1 means every time step
        "divideNeumannBoundaryConditionValuesByTotalArea": True,            # if the given Neumann boundary condition values under "neumannBoundaryConditions" are total forces instead of surface loads and therefore should be scaled by the surface area of all elements where Neumann BC are applied
        
        "initialValuesDisplacements":  [[0.0,0.0,0.0] for _ in range(mx*my*mz)],     # the initial values for the displacements, vector of values for every node [[node1-x,y,z], [node2-x,y,z], ...]
        "initialValuesVelocities":     [[0.0,0.0,0.0] for _ in range(mx*my*mz)],     # the initial values for the velocities, vector of values for every node [[node1-x,y,z], [node2-x,y,z], ...]
        "constantBodyForce":           variables.constant_body_force,                 # a constant force that acts on the whole body, e.g. for gravity
        
        "dirichletOutputFilename":     None,                                    # filename for a vtp file that contains the Dirichlet boundary condition nodes and their values, set to None to disable
        
        # define which file formats should be written
        # 1. main output writer that writes output files using the quadratic elements function space. Writes displacements, velocities and PK2 stresses.
        "OutputWriter" : [
          
          # Paraview files
          #{"format": "Paraview", "outputInterval": 1, "filename": "out/u", "binary": False, "fixedFormat": False, "onlyNodalValues":True, "combineFiles":True},
          
          # Python callback function "postprocess"
          #{"format": "PythonCallback", "outputInterval": 1, "callback": postprocess, "onlyNodalValues":True, "filename": ""},
        ],
        # 2. additional output writer that writes also the hydrostatic pressure
        "pressure": {   # output files for pressure function space (linear elements), contains pressure values, as well as displacements and velocities
          "OutputWriter" : [
            #{"format": "Paraview", "outputInterval": 1, "filename": "out/p", "binary": False, "fixedFormat": False, "onlyNodalValues":True, "combineFiles":True},
          ]
        },
        # 3. additional output writer that writes virtual work terms
        "dynamic": {    # output of the dynamic solver, has additional virtual work values 
          "OutputWriter" : [   # output files for displacements function space (quadratic elements)
            #{"format": "Paraview", "outputInterval": int(output_interval/dt), "filename": "out/dynamic", "binary": False, "fixedFormat": False, "onlyNodalValues":True, "combineFiles":True},
            #{"format": "Paraview", "outputInterval": 1, "filename": "out/dynamic", "binary": False, "fixedFormat": False, "onlyNodalValues":True, "combineFiles":True},
          ],
        }
      },
      "HyperelasticitySolver": {
        "durationLogKey":             "hyperelasticity",         # key to find duration of this solver in the log file
        "timeStepOutputInterval":     1,                         # how often the current time step should be printed to console
        
        "materialParameters":         variables.material_parameters,  # material parameters of the Mooney-Rivlin material
        "density":                    variables.rho,             # density of the material
        "displacementsScalingFactor": 1.0,                       # scaling factor for displacements, only set to sth. other than 1 only to increase visual appearance for very small displacements
        "residualNormLogFilename":    "log_residual_norm.txt",   # log file where residual norm values of the nonlinear solver will be written
        "useAnalyticJacobian":        True,                      # whether to use the analytically computed jacobian matrix in the nonlinear solver (fast)
        "useNumericJacobian":         False,                     # whether to use the numerically computed jacobian matrix in the nonlinear solver (slow), only works with non-nested matrices, if both numeric and analytic are enable, it uses the analytic for the preconditioner and the numeric as normal jacobian
          
        "dumpDenseMatlabVariables":   False,                     # whether to have extra output of matlab vectors, x,r, jacobian matrix (very slow)
        # if useAnalyticJacobian,useNumericJacobian and dumpDenseMatlabVariables all all three true, the analytic and numeric jacobian matrices will get compared to see if there are programming errors for the analytic jacobian
        
        #"loadFactors":  [0.1, 0.2, 0.35, 0.5, 1.0],             # load factors for every timestep
        "loadFactors":                [],                        # no load factors, solve problem directly
        "loadFactorGiveUpThreshold":  4e-2,                      # a threshold for the load factor, when to abort the solve of the current time step. The load factors are adjusted automatically if the nonlinear solver diverged. If the progression between two subsequent load factors gets smaller than this value, the solution is aborted.
        "scaleInitialGuess":          False,                     # when load stepping is used, scale initial guess between load steps a and b by sqrt(a*b)/a. This potentially reduces the number of iterations per load step (but not always).
        "nNonlinearSolveCalls": 1,                               # how often the nonlinear solve should be repeated
    
        # mesh
        "inputMeshIsGlobal":          False,                     # the mesh is given locally
        "meshName":                   "3Dmesh_quadratic",        # name of the 3D mesh, it is defined under "Meshes" at the beginning of this config
        "fiberMeshNames":             [],                        # fiber meshes that will be used to determine the fiber direction, for multidomain there are no fibers so this would be empty list
        "fiberDirection":             None,                      # if fiberMeshNames is empty, directly set the constant fiber direction, in element coordinate system
        "fiberDirectionInElement":    [0,0,1],                   # if fiberMeshNames is empty, directly set the constant fiber direction, in element coordinate system
      
        # solver
        "solverName":                 "mechanicsSolver",         # name of the nonlinear solver configuration, it is defined under "Solvers" at the beginning of this config
        
        # boundary and initial conditions
        "dirichletBoundaryConditions": variables.elasticity_dirichlet_bc,   # the initial Dirichlet boundary conditions that define values for displacements u and velocity v
        "neumannBoundaryConditions":   variables.elasticity_neumann_bc,     # Neumann boundary conditions that define traction forces on surfaces of elements
        "updateDirichletBoundaryConditionsFunction": None,                  # function that updates the dirichlet BCs while the simulation is running
        "updateDirichletBoundaryConditionsFunctionCallInterval": 1,         # every which step the update function should be called, 1 means every time step
        "divideNeumannBoundaryConditionValuesByTotalArea": True,            # if the given Neumann boundary condition values under "neumannBoundaryConditions" are total forces instead of surface loads and therefore should be scaled by the surface area of all elements where Neumann BC are applied
        
        "initialValuesDisplacements":  [[0.0,0.0,0.0] for _ in range(mx*my*mz)],     # the initial values for the displacements, vector of values for every node [[node1-x,y,z], [node2-x,y,z], ...]
        "constantBodyForce":           variables.constant_body_force,                 # a constant force that acts on the whole body, e.g. for gravity
        
        "dirichletOutputFilename":     "out/"+variables.scenario_name+"/dirichlet_boundary_conditions",    # filename for a vtp file that contains the Dirichlet boundary condition nodes and their values, set to None to disable
        
        # define which file formats should be written
        # 1. main output writer that writes output files using the quadratic elements function space. Writes displacements, velocities and PK2 stresses.
        "OutputWriter" : [
          
          # Paraview files
          #{"format": "Paraview", "outputInterval": 1, "filename": "out/u", "binary": False, "fixedFormat": False, "onlyNodalValues":True, "combineFiles":True},
          
          # Python callback function "postprocess"
          #{"format": "PythonCallback", "outputInterval": 1, "callback": postprocess, "onlyNodalValues":True, "filename": ""},
        ],
        # 2. additional output writer that writes also the hydrostatic pressure
        "pressure": {   # output files for pressure function space (linear elements), contains pressure values, as well as displacements and velocities
          "OutputWriter" : [
            #{"format": "Paraview", "outputInterval": 1, "filename": "out/p", "binary": False, "fixedFormat": False, "onlyNodalValues":True, "combineFiles":True},
          ]
        },
        # 3. additional output writer that writes virtual work terms
        "dynamic": {    # output of the dynamic solver, has additional virtual work values 
          "OutputWriter" : [   # output files for displacements function space (quadratic elements)
            #{"format": "Paraview", "outputInterval": int(output_interval/dt), "filename": "out/dynamic", "binary": False, "fixedFormat": False, "onlyNodalValues":True, "combineFiles":True},
            #{"format": "Paraview", "outputInterval": 1, "filename": "out/dynamic", "binary": False, "fixedFormat": False, "onlyNodalValues":True, "combineFiles":True},
          ],
        },
        # output writer for debugging, outputs files after each load increment, the geometry is not changed but u and v are written
        "LoadIncrements": {   
          "OutputWriter" : [
            #{"format": "Paraview", "outputInterval": 1, "filename": "out/load_increment_", "binary": False, "fixedFormat": False, "onlyNodalValues":True, "combineFiles":True},
          ]
        },
      }
    }
  }
}

# stop timer and calculate how long parsing lasted
if rank_no == 0:
  t_stop_script = timeit.default_timer()
  print("Python config parsed in {:.1f}s.".format(t_stop_script - t_start_script))