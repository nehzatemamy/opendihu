#include "specialized_solver/fast_monodomain_solver/fast_monodomain_solver_base.h"

#include "partition/rank_subset.h"
#include "control/diagnostic_tool/stimulation_logging.h"

//! get element lengths and vmValues from the other ranks
template<int nStates, int nIntermediates>
void FastMonodomainSolverBase<nStates,nIntermediates>::
fetchFiberData()
{
  VLOG(1) << "fetchFiberData";
  std::vector<typename NestedSolversType::TimeSteppingSchemeType> &instances = nestedSolvers_.instancesLocal();

  // loop over fibers and communicate element lengths and initial values to the ranks that participate in computing

  int fiberNo = 0;
  int fiberDataNo = 0;
  for (int i = 0; i < instances.size(); i++)
  {
    std::vector<TimeSteppingScheme::Heun<CellmlAdapterType>> &innerInstances
      = instances[i].timeStepping1().instancesLocal();  // TimeSteppingScheme::Heun<CellmlAdapter...

    for (int j = 0; j < innerInstances.size(); j++, fiberNo++)
    {
      std::shared_ptr<FiberFunctionSpace> fiberFunctionSpace = innerInstances[j].data().functionSpace();
      LOG(DEBUG) << "(" << i << "," << j << ") functionSpace " << fiberFunctionSpace->meshName();

      // communicate element lengths
      std::vector<double> localLengths(fiberFunctionSpace->nElementsLocal());

      // loop over local elements and compute element lengths
      for (element_no_t elementNoLocal = 0; elementNoLocal < fiberFunctionSpace->nElementsLocal(); elementNoLocal++)
      {
        std::array<Vec3, FiberFunctionSpace::nDofsPerElement()> geometryElementValues;
        fiberFunctionSpace->geometryField().getElementValues(elementNoLocal, geometryElementValues);
        double elementLength = MathUtility::distance<3>(geometryElementValues[0], geometryElementValues[1]);
        localLengths[elementNoLocal] = elementLength;
      }

      std::shared_ptr<Partition::RankSubset> rankSubset = fiberFunctionSpace->meshPartition()->rankSubset();
      MPI_Comm mpiCommunicator = rankSubset->mpiCommunicator();
      int computingRank = fiberNo % rankSubset->size();

      std::vector<int> nElementsOnRanks(rankSubset->size());
      std::vector<int> nDofsOnRanks(rankSubset->size());
      std::vector<int> offsetsOnRanks(rankSubset->size());

      double *elementLengthsReceiveBuffer = nullptr;
      double *vmValuesReceiveBuffer = nullptr;

      if (computingRank == rankSubset->ownRankNo())
      {
        fiberData_[fiberDataNo].elementLengths.resize(fiberFunctionSpace->nElementsGlobal());
        fiberData_[fiberDataNo].vmValues.resize(fiberFunctionSpace->nDofsGlobal());

        // resize buffer of further data that will be transferred back in updateFiberData()
        int nStatesAndIntermediatesValues = statesForTransfer_.size() + intermediatesForTransfer_.size() - 1;
        fiberData_[fiberDataNo].furtherStatesAndIntermediatesValues.resize(fiberFunctionSpace->nDofsGlobal() * nStatesAndIntermediatesValues);
        
        elementLengthsReceiveBuffer = fiberData_[fiberDataNo].elementLengths.data();
        vmValuesReceiveBuffer = fiberData_[fiberDataNo].vmValues.data();
      }

      for (int rankNo = 0; rankNo < rankSubset->size(); rankNo++)
      {
        nElementsOnRanks[rankNo] = fiberFunctionSpace->meshPartition()->nNodesLocalWithGhosts(0, rankNo) - 1;
        offsetsOnRanks[rankNo] = fiberFunctionSpace->meshPartition()->beginNodeGlobalNatural(0, rankNo);
        nDofsOnRanks[rankNo] = fiberFunctionSpace->meshPartition()->nNodesLocalWithoutGhosts(0, rankNo);
      }

      // int MPI_Gatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
      //          void *recvbuf, const int *recvcounts, const int *displs,
      //          MPI_Datatype recvtype, int root, MPI_Comm comm)
      //
      VLOG(1) << "Gatherv of element lengths to rank " << computingRank << ", values " << localLengths << ", sizes: " << nElementsOnRanks << ", offsets: " << offsetsOnRanks;

      MPI_Gatherv(localLengths.data(), fiberFunctionSpace->nElementsLocal(), MPI_DOUBLE,
                  elementLengthsReceiveBuffer, nElementsOnRanks.data(), offsetsOnRanks.data(),
                  MPI_DOUBLE, computingRank, mpiCommunicator);

      // communicate Vm values
      std::vector<double> vmValuesLocal;
      innerInstances[j].data().solution()->getValuesWithoutGhosts(0, vmValuesLocal);

      LOG(DEBUG) << "Gatherv of values to rank " << computingRank << ", sizes: " << nDofsOnRanks << ", offsets: " << offsetsOnRanks << ", local values " << vmValuesLocal;

      MPI_Gatherv(vmValuesLocal.data(), fiberFunctionSpace->nDofsLocalWithoutGhosts(), MPI_DOUBLE,
                  vmValuesReceiveBuffer, nDofsOnRanks.data(), offsetsOnRanks.data(),
                  MPI_DOUBLE, computingRank, mpiCommunicator);

      // increase index for fiberData_ struct
      if (computingRank == rankSubset->ownRankNo())
        fiberDataNo++;
    }
  }

  // copy Vm values to compute buffers
  for (int fiberDataNo = 0; fiberDataNo < fiberData_.size(); fiberDataNo++)
  {
    int nValues = fiberData_[fiberDataNo].vmValues.size();

    for (int valueNo = 0; valueNo < nValues; valueNo++)
    {
      global_no_t valueIndexAllFibers = fiberData_[fiberDataNo].valuesOffset + valueNo;

      global_no_t pointBuffersNo = valueIndexAllFibers / Vc::double_v::Size;
      int entryNo = valueIndexAllFibers % Vc::double_v::Size;

      fiberPointBuffers_[pointBuffersNo].states[0][entryNo] = fiberData_[fiberDataNo].vmValues[valueNo];
    }
  }
}

//! send vmValues data from fiberData_ back to the fibers where it belongs to and set in the respective field variable
template<int nStates, int nIntermediates>
void FastMonodomainSolverBase<nStates,nIntermediates>::
updateFiberData()
{
  // copy Vm and other states/intermediates from compute buffers to fiberData_
  for (int fiberDataNo = 0; fiberDataNo < fiberData_.size(); fiberDataNo++)
  {
    int nValues = fiberData_[fiberDataNo].vmValues.size();

    for (int valueNo = 0; valueNo < nValues; valueNo++)
    {
      global_no_t valueIndexAllFibers = fiberData_[fiberDataNo].valuesOffset + valueNo;

      global_no_t pointBuffersNo = valueIndexAllFibers / Vc::double_v::Size;
      int entryNo = valueIndexAllFibers % Vc::double_v::Size;

      assert(statesForTransfer_.size() > 0);
      const int stateToTransfer = statesForTransfer_[0];  // transfer the first state value
      fiberData_[fiberDataNo].vmValues[valueNo] = fiberPointBuffers_[pointBuffersNo].states[stateToTransfer][entryNo];

      // loop over further states to transfer
      int furtherDataIndex = 0;
      for (int i = 1; i < statesForTransfer_.size(); i++, furtherDataIndex++)
      {
        const int stateToTransfer = statesForTransfer_[i];

        fiberData_[fiberDataNo].furtherStatesAndIntermediatesValues[furtherDataIndex*nValues + valueNo]
          = fiberPointBuffers_[pointBuffersNo].states[stateToTransfer][entryNo];
      }

      // loop over intermediates to transfer
      for (int i = 0; i < intermediatesForTransfer_.size(); i++, furtherDataIndex++)
      {
        fiberData_[fiberDataNo].furtherStatesAndIntermediatesValues[furtherDataIndex*nValues + valueNo]
          = fiberPointBuffersIntermediatesForTransfer_[pointBuffersNo][i][entryNo];
      }
    }
    LOG(DEBUG) << "states and intermediates for transfer at fiberDataNo=" << fiberDataNo << ": " << fiberData_[fiberDataNo].furtherStatesAndIntermediatesValues;
    LOG(DEBUG) << "size: " << fiberData_[fiberDataNo].furtherStatesAndIntermediatesValues.size() << ", nValues: " << nValues;
  }

  LOG(TRACE) << "updateFiberData";
  std::vector<typename NestedSolversType::TimeSteppingSchemeType> &instances = nestedSolvers_.instancesLocal();

  // loop over fibers and communicate element lengths and initial values to the ranks that participate in computing

  int fiberNo = 0;
  int fiberDataNo = 0;
  for (int i = 0; i < instances.size(); i++)
  {
    std::vector<TimeSteppingScheme::Heun<CellmlAdapterType>> &innerInstances
      = instances[i].timeStepping1().instancesLocal();  // TimeSteppingScheme::Heun<CellmlAdapter...

    for (int j = 0; j < innerInstances.size(); j++, fiberNo++)
    {
      std::shared_ptr<FiberFunctionSpace> fiberFunctionSpace = innerInstances[j].data().functionSpace();
      //CellmlAdapterType &cellmlAdapter = innerInstances[j].discretizableInTime();

      // prepare helper variables for Scatterv
      std::shared_ptr<Partition::RankSubset> rankSubset = fiberFunctionSpace->meshPartition()->rankSubset();
      MPI_Comm mpiCommunicator = rankSubset->mpiCommunicator();
      int computingRank = fiberNo % rankSubset->size();

      std::vector<int> nDofsOnRanks(rankSubset->size());
      std::vector<int> offsetsOnRanks(rankSubset->size());

      for (int rankNo = 0; rankNo < rankSubset->size(); rankNo++)
      {
        offsetsOnRanks[rankNo] = fiberFunctionSpace->meshPartition()->beginNodeGlobalNatural(0, rankNo);
        nDofsOnRanks[rankNo] = fiberFunctionSpace->meshPartition()->nNodesLocalWithoutGhosts(0, rankNo);
      }

      double *sendBufferVmValues = nullptr;
      if (computingRank == rankSubset->ownRankNo())
      {
        sendBufferVmValues = fiberData_[fiberDataNo].vmValues.data();
      }

      //  int MPI_Scatterv(const void *sendbuf, const int *sendcounts, const int *displs, MPI_Datatype sendtype,
      //                  void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm)
      // communicate Vm values
      std::vector<double> vmValuesLocal(fiberFunctionSpace->nDofsLocalWithoutGhosts());
      MPI_Scatterv(sendBufferVmValues, nDofsOnRanks.data(), offsetsOnRanks.data(), MPI_DOUBLE,
                   vmValuesLocal.data(), fiberFunctionSpace->nDofsLocalWithoutGhosts(), MPI_DOUBLE,
                   computingRank, mpiCommunicator);

      VLOG(1) << "Scatterv from rank " << computingRank << ", sizes: " << nDofsOnRanks << ", offsets: " << offsetsOnRanks << ", received local values " << vmValuesLocal;

      // store Vm values in CellmlAdapter and diffusion FiniteElementMethod
      VLOG(1) << "fiber " << fiberDataNo << ", set values " << vmValuesLocal;
      innerInstances[j].data().solution()->setValuesWithoutGhosts(0, vmValuesLocal);
      instances[i].timeStepping2().instancesLocal()[j].data().solution()->setValuesWithoutGhosts(0, vmValuesLocal);

      // ----------------------
      // communicate further states and intermediates that are selected by the options "statesForTransfer" and "intermediatesForTransfer"

      std::vector<int> nValuesOnRanks(rankSubset->size());
      int nStatesAndIntermediatesValues = statesForTransfer_.size() + intermediatesForTransfer_.size() - 1;

      for (int rankNo = 0; rankNo < rankSubset->size(); rankNo++)
      {
        offsetsOnRanks[rankNo] = fiberFunctionSpace->meshPartition()->beginNodeGlobalNatural(0, rankNo) * nStatesAndIntermediatesValues;
        nValuesOnRanks[rankNo] = fiberFunctionSpace->meshPartition()->nNodesLocalWithoutGhosts(0, rankNo) * nStatesAndIntermediatesValues;
      }

      double *sendBuffer = nullptr;
      if (computingRank == rankSubset->ownRankNo())
      {
        sendBuffer = fiberData_[fiberDataNo].furtherStatesAndIntermediatesValues.data();
      }

      std::vector<double> valuesLocal(fiberFunctionSpace->nDofsLocalWithoutGhosts() * nStatesAndIntermediatesValues);
      MPI_Scatterv(sendBuffer, nValuesOnRanks.data(), offsetsOnRanks.data(), MPI_DOUBLE,
                   valuesLocal.data(), fiberFunctionSpace->nDofsLocalWithoutGhosts() * nStatesAndIntermediatesValues, MPI_DOUBLE,
                   computingRank, mpiCommunicator);

      VLOG(1) << "Scatterv furtherStatesAndIntermediatesValues from rank " << computingRank << ", sizes: " << nValuesOnRanks << ", offsets: " << offsetsOnRanks
        << ", sendBuffer: " << sendBuffer << ", received local values: " << valuesLocal;

      // store received states and intermediates values in diffusion outputConnectorData
      // loop over further states to transfer
      int furtherDataIndex = 0;
      for (int stateIndex = 1; stateIndex < statesForTransfer_.size(); stateIndex++, furtherDataIndex++)
      {
        // store in diffusion

        // get field variable
        std::vector<::Data::ComponentOfFieldVariable<FiberFunctionSpace,1>> &variable1
          = instances[i].timeStepping2().instancesLocal()[j].getOutputConnectorData()->variable1;

        if (stateIndex >= variable1.size())
        {
          continue;
        }
        std::shared_ptr<FieldVariable::FieldVariable<FiberFunctionSpace,1>> fieldVariableStates
          = variable1[stateIndex].values;

        int nValues = fiberFunctionSpace->nDofsLocalWithoutGhosts();
        double *values = valuesLocal.data() + furtherDataIndex * nValues;

        // int componentNo, int nValues, const dof_no_t *dofNosLocal, const double *values
        fieldVariableStates->setValues(0, nValues, fiberFunctionSpace->meshPartition()->dofNosLocal().data(), values);

        // store in cellmlAdapter
        std::shared_ptr<FieldVariable::FieldVariable<FiberFunctionSpace,nStates>> fieldVariableStatesCellML
          = instances[i].timeStepping1().instancesLocal()[j].getOutputConnectorData()->variable1[stateIndex].values;

        const int componentNo = statesForTransfer_[stateIndex];

        // int componentNo, int nValues, const dof_no_t *dofNosLocal, const double *values
        fieldVariableStatesCellML->setValues(componentNo, nValues, fiberFunctionSpace->meshPartition()->dofNosLocal().data(), values);

        VLOG(1) << "store " << nValues << " values for additional state " << statesForTransfer_[stateIndex];
      }

      // loop over intermediates to transfer
      for (int intermediateIndex = 0; intermediateIndex < intermediatesForTransfer_.size(); intermediateIndex++, furtherDataIndex++)
      {
        // store in diffusion

        // get field variable
        std::vector<::Data::ComponentOfFieldVariable<FiberFunctionSpace,1>> &variable2
          = instances[i].timeStepping2().instancesLocal()[j].getOutputConnectorData()->variable2;

        if (intermediateIndex >= variable2.size())
        {
          continue;
        }

        std::shared_ptr<FieldVariable::FieldVariable<FiberFunctionSpace,1>> fieldVariableIntermediates
          = variable2[intermediateIndex].values;

        int nValues = fiberFunctionSpace->nDofsLocalWithoutGhosts();
        double *values = valuesLocal.data() + furtherDataIndex * nValues;

        // int componentNo, int nValues, const dof_no_t *dofNosLocal, const double *values
        fieldVariableIntermediates->setValues(0, nValues, fiberFunctionSpace->meshPartition()->dofNosLocal().data(), values);

        // store in CellmlAdapter
        std::shared_ptr<FieldVariable::FieldVariable<FiberFunctionSpace,1>> fieldVariableIntermediatesCellML
          = instances[i].timeStepping1().instancesLocal()[j].getOutputConnectorData()->variable2[intermediateIndex].values;

        //const int componentNo = intermediatesForTransfer_[intermediateIndex];

        // int componentNo, int nValues, const dof_no_t *dofNosLocal, const double *values
        fieldVariableIntermediatesCellML->setValues(0, nValues, fiberFunctionSpace->meshPartition()->dofNosLocal().data(), values);

        LOG(DEBUG) << "store " << nValues << " values for intermediate " << intermediatesForTransfer_[intermediateIndex];
        LOG(DEBUG) << *fieldVariableIntermediates;
      }

      // increase index for fiberData_ struct
      if (computingRank == rankSubset->ownRankNo())
        fiberDataNo++;
    }
  }
}
