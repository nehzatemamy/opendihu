#include "partition/partitioned_petsc_mat/partitioned_petsc_mat_base.h"


template<typename RowsFunctionSpaceType,typename ColumnsFunctionSpaceType>
PartitionedPetscMatBase<RowsFunctionSpaceType,ColumnsFunctionSpaceType>::
PartitionedPetscMatBase(std::shared_ptr<Partition::MeshPartition<RowsFunctionSpaceType>> meshPartitionRows,
                        std::shared_ptr<Partition::MeshPartition<ColumnsFunctionSpaceType>> meshPartitionColumns, std::string name) :
  meshPartitionRows_(meshPartitionRows), meshPartitionColumns_(meshPartitionColumns), name_(name)
{
}


template<typename RowsFunctionSpaceType,typename ColumnsFunctionSpaceType>
std::shared_ptr<Partition::MeshPartition<RowsFunctionSpaceType>>
PartitionedPetscMatBase<RowsFunctionSpaceType,ColumnsFunctionSpaceType>::  
meshPartitionRows()
{
  return meshPartitionRows_;
}

template<typename RowsFunctionSpaceType,typename ColumnsFunctionSpaceType>
std::shared_ptr<Partition::MeshPartition<ColumnsFunctionSpaceType>>
PartitionedPetscMatBase<RowsFunctionSpaceType,ColumnsFunctionSpaceType>::
meshPartitionColumns()
{
  return meshPartitionColumns_;
}
