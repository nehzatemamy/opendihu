#include "partition/partitioned_petsc_vec.h"

template<typename BasisOnMeshType, int nComponents, typename DummyForTraits>
PartitionedPetscVec<BasisOnMeshType,nComponents,DummyForTraits>::
PartitionedPetscVec(std::shared_ptr<Partition::MeshPartition<BasisOnMeshType>> meshPartition, std::string name) :
  PartitionedPetscVecBase<BasisOnMeshType>(meshPartition, name)
{
  createVector();
}

//! constructor, copy from existing vector
template<typename BasisOnMeshType, int nComponents, typename DummyForTraits>
PartitionedPetscVec<BasisOnMeshType,nComponents,DummyForTraits>::
PartitionedPetscVec(PartitionedPetscVec<BasisOnMeshType,nComponents> &rhs, std::string name) :
  PartitionedPetscVecBase<BasisOnMeshType>(rhs.meshPartition_, name)
 
{
  createVector();
}

template<typename BasisOnMeshType, int nComponents, typename DummyForTraits>
void PartitionedPetscVec<BasisOnMeshType, nComponents, DummyForTraits>::
createVector()
{
  PetscErrorCode ierr;
  
  assert(this->meshPartition_);
  
  dof_no_t nDofsPerNode = BasisOnMeshType::nDofsPerNode();
  dof_no_t nEntriesLocal = this->meshPartition_->nNodesLocal() * nDofsPerNode;
  dof_no_t nEntriesGlobal = this->meshPartition_->nNodesGlobal() * nDofsPerNode;
  
  // loop over the components of this field variable
  for (int componentNo = 0; componentNo < nComponents; componentNo++)
  {
    // initialize PETSc vector object
    ierr = VecCreate(this->meshPartition_->mpiCommunicator(), &values_[componentNo]); CHKERRV(ierr);
    ierr = PetscObjectSetName((PetscObject) values_[componentNo], this->name_.c_str()); CHKERRV(ierr);

    // initialize size of vector
    ierr = VecSetSizes(values_[componentNo], nEntriesLocal, nEntriesGlobal); CHKERRV(ierr);

    // set sparsity type and other options
    ierr = VecSetFromOptions(values_[componentNo]); CHKERRV(ierr);
  }
}

//! this has to be called before the vector is manipulated (i.e. VecSetValues or vecZeroEntries is called)
template<typename BasisOnMeshType, int nComponents, typename DummyForTraits>
void PartitionedPetscVec<BasisOnMeshType, nComponents, DummyForTraits>::
startVectorManipulation()
{
}

//! this has to be called after the vector is manipulated (i.e. VecSetValues or vecZeroEntries is called)
template<typename BasisOnMeshType, int nComponents, typename DummyForTraits>
void PartitionedPetscVec<BasisOnMeshType, nComponents, DummyForTraits>::
finishVectorManipulation()
{
  PetscErrorCode ierr;
  // loop over the components of this field variable
  for (int componentNo = 0; componentNo < nComponents; componentNo++)
  {
    ierr = VecAssemblyBegin(values_[componentNo]); CHKERRV(ierr); 
  }
  
  // loop over the components of this field variable
  for (int componentNo = 0; componentNo < nComponents; componentNo++)
  {
    ierr = VecAssemblyEnd(values_[componentNo]); CHKERRV(ierr);
  }
}

//! wrapper to the PETSc VecSetValues, acting only on the local data, the indices ix are the local dof nos
template<typename BasisOnMeshType, int nComponents, typename DummyForTraits>
void PartitionedPetscVec<BasisOnMeshType, nComponents, DummyForTraits>::
setValues(int componentNo, PetscInt ni, const PetscInt ix[], const PetscScalar y[], InsertMode iora)
{
  // this wraps the standard PETSc VecSetValues on the local vector
  PetscErrorCode ierr;
  ierr = VecSetValues(values_[componentNo], ni, ix, y, iora); CHKERRV(ierr);
}

//! wrapper to the PETSc VecSetValue, acting only on the local data
template<typename BasisOnMeshType, int nComponents, typename DummyForTraits>
void PartitionedPetscVec<BasisOnMeshType, nComponents, DummyForTraits>::
setValue(int componentNo, PetscInt row, PetscScalar value, InsertMode mode)
{
  // this wraps the standard PETSc VecSetValue on the local vector
  PetscErrorCode ierr;
  ierr = VecSetValue(values_[componentNo], row, value, mode); CHKERRV(ierr);
}
/*
//! for a single component vector set all values
template<typename BasisOnMeshType, int nComponents, typename DummyForTraits>
void PartitionedPetscVec<BasisOnMeshType, nComponents, DummyForTraits>::
setValuesWithGhosts(int componentNo, const std::vector<double> &values, InsertMode petscInsertMode)
{
  assert(values.size() == this->meshPartition_->nNodesLocalWithGhosts());
 
  // this wraps the standard PETSc VecSetValue on the local vector
  PetscErrorCode ierr;
  ierr = VecSetValue(values_[componentNo], this->meshPartition_->localDofNos().data(), values.data(), petscInsertMode); CHKERRV(ierr);
}

//! for a single component vector set all values
template<typename BasisOnMeshType, int nComponents, typename DummyForTraits>
void PartitionedPetscVec<BasisOnMeshType, nComponents, DummyForTraits>::
setValuesWithoutGhosts(int componentNo, const std::vector<double> &values, InsertMode petscInsertMode)
{
  assert(values.size() == this->meshPartition_->nNodesLocalWithoutGhosts());
 
  // this wraps the standard PETSc VecSetValue on the local vector
  PetscErrorCode ierr;
  ierr = VecSetValue(values_[componentNo], this->meshPartition_->localDofNos().data(), values.data(), petscInsertMode); CHKERRV(ierr);
}
  */
//! wrapper to the PETSc VecGetValues, acting only on the local data, the indices ix are the local dof nos
template<typename BasisOnMeshType, int nComponents, typename DummyForTraits>
void PartitionedPetscVec<BasisOnMeshType, nComponents, DummyForTraits>::
getValues(int componentNo, PetscInt ni, const PetscInt ix[], PetscScalar y[])
{
  // this wraps the standard PETSc VecGetValues on the local vector
  PetscErrorCode ierr;
  ierr = VecGetValues(values_[componentNo], ni, ix, y); CHKERRV(ierr); 
}

//! wrapper to the PETSc VecGetValues, on the global vector with global indexing
template<typename BasisOnMeshType, int nComponents, typename DummyForTraits>
void PartitionedPetscVec<BasisOnMeshType, nComponents, DummyForTraits>::
getValuesGlobalIndexing(int componentNo, PetscInt ni, const PetscInt ix[], PetscScalar y[])
{
  getValues(componentNo, ni, ix, y);
}

//! set all entries to zero, wraps VecZeroEntries
template<typename BasisOnMeshType, int nComponents, typename DummyForTraits>
void PartitionedPetscVec<BasisOnMeshType, nComponents, DummyForTraits>::
zeroEntries()
{
  PetscErrorCode ierr;
  for (int componentNo = 0; componentNo < nComponents; componentNo++)
  {
    ierr = VecZeroEntries(values_[componentNo]); CHKERRV(ierr);
  }
}

//! get the local Vector of a specified component
template<typename BasisOnMeshType, int nComponents, typename DummyForTraits>
Vec &PartitionedPetscVec<BasisOnMeshType, nComponents, DummyForTraits>::
valuesLocal(int componentNo)
{
  return values_[componentNo];
}

//! get the global Vector of a specified component
template<typename BasisOnMeshType, int nComponents, typename DummyForTraits>
Vec &PartitionedPetscVec<BasisOnMeshType, nComponents, DummyForTraits>::
valuesGlobal(int componentNo)
{
  return values_[componentNo];
}

template<typename BasisOnMeshType, int nComponents, typename DummyForTraits>
void PartitionedPetscVec<BasisOnMeshType, nComponents, DummyForTraits>::
output(std::ostream &stream)
{
#ifndef NDEBUG  
  // this method gets all values and outputs them to stream, only on rank 0
  PetscMPIInt ownRankNo, nRanks;
  MPI_Comm_rank(MPI_COMM_WORLD, &ownRankNo);
  MPI_Comm_size(PETSC_COMM_WORLD, &nRanks);
    
  for (int componentNo = 0; componentNo < nComponents; componentNo++)
  {
    // get global size of matrix 
    int nRows, nColumns, nRowsLocal, nColumnsLocal;
    PetscErrorCode ierr;
    ierr = MatGetSize(values_[componentNo], &nRows, &nColumns); CHKERRV(ierr);
    ierr = MatGetLocalSize(values_[componentNo], &nRowsLocal, &nColumnsLocal); CHKERRV(ierr);
    
    // retrieve local values
    int nDofsLocal = this->meshPartition_->nDofsLocalWithoutGhosts();
    std::vector<double> localValues(nDofsLocal);
    
    VecGetValuesLocal(values_[componentNo], nDofsLocal, this->meshPartition_->dofNosLocal(), localValues.data());
    
    std::vector<int> localSizes(nRanks);
    localSizes[ownRankNo] = nDofsLocal;
    MPI_Gather(localSizes.data() + ownRankNo, 1, MPI_INT, localSizes.data(), nRanks, MPI_INT, 0, this->meshPartition_->mpiCommunicator());
    
    int maxLocalSize;
    MPI_Allreduce(localSizes.data() + ownRankNo, &maxLocalSize, 1, MPI_INT, MPI_MAX, this->meshPartition_->mpiCommunicator());
    
    std::vector<double> recvBuffer(maxLocalSize*nRanks);
    std::vector<double> sendBuffer(maxLocalSize,0.0);
    std::copy(localSizes.begin(), localSizes.end(), sendBuffer.begin());
    
    MPI_Gather(sendBuffer.data(), maxLocalSize, MPI_DOUBLE, recvBuffer.data(), maxLocalSize*nRanks, MPI_DOUBLE, 0, this->meshPartition_->mpiCommunicator());
    
    if (ownRankNo == 0)
    {
      stream << "vector \"" << this->name_ << "\"" << std::endl;
      stream << "component " << componentNo << ": ";
      for (int rankNo = 0; rankNo < nRanks; rankNo++)
      {
        for (int i = 0; i < localSizes[rankNo]; i++)
        {
          stream << recvBuffer[rankNo*maxLocalSize + i] << "  ";
        }
      }
      stream << std::endl; 
    }
  }
#endif
}

template<typename BasisOnMeshType, int nComponents>
std::ostream &operator<<(std::ostream &stream, PartitionedPetscVec<BasisOnMeshType,nComponents> &vector)
{
  vector.output(stream);
  return stream;
}