// most important parallel-in-time structures and functions for XBraid
// more functions in PinT_lib.h

#pragma once

#include <braid.h>

#include "time_stepping_scheme/heun.h"
//#include "time_stepping_scheme/implicit_euler.h"
#include "spatial_discretization/finite_element_method/finite_element_method.h"
#include "basis_function/lagrange.h"
#include "mesh/structured_regular_fixed.h"
#include "specialized_solver/multidomain_solver/multidomain_solver.h"
#include "operator_splitting/strang.h"
#include "control/multiple_instances.h"
#include "specialized_solver/parallel_in_time/MultiDomain/multidomain_wrapper.h"


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <vector>

#include <petscdm.h>
#include <petscdmda.h>
#include <petscts.h>
#include <petscdraw.h>
#include <petscvec.h>

typedef struct _braid_App_struct
{
  MPI_Comm  comm;
  double    tstart;       /* Define the temporal domain */
  double    tstop;
  int       ntime;
  double    xstart;       /* Define the spatial domain */
  double    xstop;
  int       nspace;
  double *  sc_info;      /* Runtime information that tracks the space-time grids visited */
  int       print_level;  /* Level of output desired by user (see the -help message below) */
  //VecScatter vecscatter;
  //int testscatter;
  typedef Mesh::StructuredDeformableOfDimension<3> MeshType;

  typedef MultidomainWrapper<
    OperatorSplitting::Strang<
      Control::MultipleInstances<
        TimeSteppingScheme::Heun<
          CellmlAdapter<
            4,9,  // nStates,nIntermediates: 57,1 = Shorten, 4,9 = Hodgkin Huxley
            FunctionSpace::FunctionSpace<MeshType,BasisFunction::LagrangeOfOrder<1>>  // same function space as for anisotropic diffusion
          >
        >
      >,
      TimeSteppingScheme::MultidomainSolver<              // multidomain
        SpatialDiscretization::FiniteElementMethod<       //FEM for initial potential flow, fibre directions
          MeshType,
          BasisFunction::LagrangeOfOrder<1>,
          Quadrature::Gauss<3>,
          Equation::Static::Laplace
        >,
        SpatialDiscretization::FiniteElementMethod<   // anisotropic diffusion
          MeshType,
          BasisFunction::LagrangeOfOrder<1>,
          Quadrature::Gauss<5>,
          Equation::Dynamic::DirectionalDiffusion
        >
      >
    >
  > NestedSolverMD;

  std::vector<
    std::shared_ptr<
      NestedSolverMD
    >
  > *MultiDomainSolvers;   //< vector of nested solvers (implicit euler) for solution on different grids
} my_App;

/* Can put anything in my vector and name it anything as well */
typedef struct _braid_Vector_struct
{
   PetscInt     size;
   PetscReal *values;
   //double *values;
   //Vec values;

} my_Vector;

/* create and allocate a vector */
void
create_vector_MD(my_Vector **u,
              int size);

int
my_Clone_MD(braid_App     app,
         braid_Vector  u,
         braid_Vector *v_ptr);

int
my_Free_MD(braid_App    app,
        braid_Vector u);

int
my_Sum_MD(braid_App     app,
       double        alpha,
       braid_Vector  x,
       double        beta,
       braid_Vector  y);

int
my_SpatialNorm_MD(braid_App     app,
               braid_Vector  u,
               double       *norm_ptr);

int
my_Access_MD(braid_App          app,
          braid_Vector       u,
          braid_AccessStatus astatus);

int
my_BufSize_MD(braid_App           app,
           int                 *size_ptr,
           braid_BufferStatus  bstatus);

int
my_BufPack_MD(braid_App           app,
           braid_Vector        u,
           void               *buffer,
           braid_BufferStatus  bstatus);

int
my_BufUnpack_MD(braid_App           app,
             void               *buffer,
             braid_Vector       *u_ptr,
             braid_BufferStatus  bstatus);

int
my_Residual_MD(braid_App        app,
            braid_Vector     ustop,
            braid_Vector     r,
            braid_StepStatus status);

/* Bilinear Coarsening */
int
my_Coarsen_MD(braid_App              app,
           braid_Vector           fu,
           braid_Vector          *cu_ptr,
           braid_CoarsenRefStatus status);

/* Bilinear interpolation */
int
my_Interp_MD(braid_App              app,
          braid_Vector           cu,
          braid_Vector          *fu_ptr,
          braid_CoarsenRefStatus status);