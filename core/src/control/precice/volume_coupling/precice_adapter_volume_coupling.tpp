#include "control/precice/volume_coupling/precice_adapter_volume_coupling.h"

namespace Control
{

template<typename NestedSolver>
void PreciceAdapterVolumeCoupling<NestedSolver>::
run()
{
#ifdef HAVE_PRECICE

  // initialize everything
  this->initialize();

  // only run the solver if precice is disabled in settings
  if (!this->couplingEnabled_)
  {
    this->nestedSolver_.run();
    return;
  }

  // assert that precice is properly initialized and the interface is available
  assert(this->preciceSolverInterface_);

  // perform initial data transfer, if required
  if (this->preciceSolverInterface_->isActionRequired(precice::constants::actionWriteInitialData()))
  {
    // writeData for this participant
    this->preciceWriteData();

    this->preciceSolverInterface_->markActionFulfilled(precice::constants::actionWriteInitialData());

    // initialize data in precice
    this->preciceSolverInterface_->initializeData();
  }

  // perform the computation of this solver
  double currentTime = 0;

  // main simulation loop of adapter
  for (int timeStepNo = 0; this->preciceSolverInterface_->isCouplingOngoing(); timeStepNo++)
  {
    if (timeStepNo % this->timeStepOutputInterval_ == 0 && (this->timeStepOutputInterval_ <= 10 || timeStepNo > 0))  // show first timestep only if timeStepOutputInterval is <= 10
    {
      LOG(INFO) << "PreCICE volume coupling, timestep " << timeStepNo << ", t=" << currentTime;
    }

    // checkpointing is not implemented in the volume coupling adapter
#if 0
    // determine if checkpoint needs to be written
    if (this->preciceSolverInterface_->isActionRequired(precice::constants::actionWriteIterationCheckpoint()))
    {
      // save checkpoint
      this->saveCheckpoint(currentTime);
      this->preciceSolverInterface_->markActionFulfilled(precice::constants::actionWriteIterationCheckpoint());
    }
#endif

    // read incoming values
    this->preciceReadData();

    // compute the time step width such that it fits in the remaining time in the current time window
    double timeStepWidth = std::min(this->maximumPreciceTimestepSize_, this->timeStepWidth_);

    // set time span in nested solver
    this->nestedSolver_.setTimeSpan(currentTime, currentTime+timeStepWidth);

    // call the nested solver to proceed with the simulation for the assigned time span
    // the parameter specifies whether the output writers are enabled
    this->nestedSolver_.advanceTimeSpan(!this->outputOnlyConvergedTimeSteps_);

    // write outgoing data to precice
    this->preciceWriteData();

    // increase current simulation time
    currentTime += timeStepWidth;

    // advance timestepping in precice
    this->maximumPreciceTimestepSize_ = this->preciceSolverInterface_->advance(timeStepWidth);

    LOG(DEBUG) << "precice::advance(" << timeStepWidth << "), maximumPreciceTimestepSize_: " << this->maximumPreciceTimestepSize_;

    // if coupling did not converge, reset to previously stored checkpoint
    // checkpointing is not implemented in the volume coupling adapter
#if 0
    if (this->preciceSolverInterface_->isActionRequired(precice::constants::actionReadIterationCheckpoint()))
    {
      // set variables back to last checkpoint
      currentTime = this->loadCheckpoint();
      this->preciceSolverInterface_->markActionFulfilled(precice::constants::actionReadIterationCheckpoint());
    }
#endif

    // if the current time step did converge and subcycling is complete
    if (this->preciceSolverInterface_->isTimeWindowComplete())
    {
      if (this->outputOnlyConvergedTimeSteps_)
      {
        // output all data in the nested solvers
        this->nestedSolver_.callOutputWriter(timeStepNo, currentTime);
      }
    }

  }   // loop over time steps

  // finalize precice interface
  this->preciceSolverInterface_->finalize();

#else
  LOG(FATAL) << "Not compiled with preCICE!";
#endif
}

template<typename NestedSolver>
void PreciceAdapterVolumeCoupling<NestedSolver>::
reset()
{
  this->nestedSolver_.reset();

  this->initialized_ = false;
  // "uninitialize" everything
}

template<typename NestedSolver>
typename PreciceAdapterVolumeCoupling<NestedSolver>::Data &PreciceAdapterVolumeCoupling<NestedSolver>::
data()
{
  // get a reference to the data object
  return this->nestedSolver_.data();
}

}  // namespace
