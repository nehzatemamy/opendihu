#pragma once

#include <memory>

#include "control/types.h"
#include "partition/rank_subset.h"
#include "function_space/function_space.h"
#include "mesh/type_traits.h"
#include "partition/partitioned_petsc_vec/01_partitioned_petsc_vec_with_dirichlet_bc.h"
#include "spatial_discretization/boundary_conditions/dirichlet_boundary_conditions.h"

/** This vector is an extension of PartitionedPetscVecWithDirichletBc (read the description there).
 *  It is a combined vector of 3 displacements components and 1 pressure component.
 */
template<typename DisplacementsFunctionSpaceType, typename PressureFunctionSpaceType>
class PartitionedPetscVecForHyperelasticity:
  public PartitionedPetscVecWithDirichletBc<DisplacementsFunctionSpaceType,4,3>
{
public:
 
  //! constructor
  PartitionedPetscVecForHyperelasticity(
    std::shared_ptr<Partition::MeshPartition<DisplacementsFunctionSpaceType>> meshPartitionDisplacements,
    std::shared_ptr<Partition::MeshPartition<PressureFunctionSpaceType>> meshPartitionPressure,
    std::shared_ptr<SpatialDiscretization::DirichletBoundaryConditions<DisplacementsFunctionSpaceType,3>> dirichletBoundaryConditions,
    std::string name
  );

  //! write the vector values to a file in natural ordering, such that the output for different numbers of ranks can be compared
  void dumpGlobalNatural(std::string filename);

  //! the meshPartition of the pressure function space, the other meshPartition, the one of the displacements function space can be obtained by meshPartition()
  std::shared_ptr<Partition::MeshPartition<PressureFunctionSpaceType>> meshPartitionPressure();

  //! get a string representation of the whole vector in global natural ordering
  //! @param horizontal if the string is for console output (less newlines) or for file output
  std::string getString(bool horizontal = true, std::string vectorName = "");

  //! get the number of displacement dofs ux,uy,uz without Dirichlet Bc constraint dofs
  dof_no_t nDisplacementDofsWithoutBcLocal();

  //! get a Petsc index set (IS) of all global non-bc indices comprising ux,uy,uz
  IS displacementDofsGlobal();

  //! get a Petsc index set (IS) of all global non-bc indices p
  IS pressureDofsGlobal();

protected:

  //! do the additional initalization after the initialization of the parent class, that accounts also for the pressure component
  void initializeForPressure();

  std::shared_ptr<Partition::MeshPartition<PressureFunctionSpaceType>> meshPartitionPressure_;   ///< the mesh partition for the pressure function space, which has a lower degree of ansatz function than the displacement function space

};

template<typename DisplacementsFunctionSpaceType, typename PressureFunctionSpaceType>
std::ostream &operator<<(std::ostream &stream, PartitionedPetscVecForHyperelasticity<DisplacementsFunctionSpaceType,PressureFunctionSpaceType> &vector);

#include "partition/partitioned_petsc_vec/02_partitioned_petsc_vec_for_hyperelasticity.tpp"