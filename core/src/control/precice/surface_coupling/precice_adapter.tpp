#include "control/precice/surface_coupling/precice_adapter.h"

#include <sstream>

namespace Control
{

template<typename NestedSolver>
void PreciceAdapter<NestedSolver>::
run()
{
#ifdef HAVE_PRECICE

  // initialize everything
  this->initialize();

  if (!this->couplingEnabled_)
  {
    this->nestedSolver_.run();
    return;
  }

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
      LOG(INFO) << "PreCICE coupling, timestep " << timeStepNo << ", t=" << currentTime;
    }

    // determine if checkpoint needs to be written
    if (this->preciceSolverInterface_->isActionRequired(precice::constants::actionWriteIterationCheckpoint()))
    {
      // save checkpoint
      this->saveCheckpoint(currentTime);
      this->preciceSolverInterface_->markActionFulfilled(precice::constants::actionWriteIterationCheckpoint());
    }

    // read incoming values
    this->preciceReadData();

    // compute the time step width such that it fits in the remaining time in the current time window
    double timeStepWidth = std::min(this->maximumPreciceTimestepSize_, this->timeStepWidth_);

    // set time span in nested solver
    this->nestedSolver_.setTimeSpan(currentTime, currentTime+timeStepWidth);

    // call the nested solver to proceed with the simulation for the assigned time span
    this->nestedSolver_.advanceTimeSpan();

    // write outgoing data to precice
    this->preciceWriteData();

    // increase current simulation time
    currentTime += timeStepWidth;

    // advance timestepping in precice
    this->maximumPreciceTimestepSize_ = this->preciceSolverInterface_->advance(timeStepWidth);

    LOG(DEBUG) << "precice::advance(" << timeStepWidth << "), maximumPreciceTimestepSize_: " << this->maximumPreciceTimestepSize_;

    // if coupling did not converge, reset to previously stored checkpoint
    if (this->preciceSolverInterface_->isActionRequired(precice::constants::actionReadIterationCheckpoint()))
    {
      // set variables back to last checkpoint
      currentTime = this->loadCheckpoint();
      this->preciceSolverInterface_->markActionFulfilled(precice::constants::actionReadIterationCheckpoint());
    }
  }   // loop over time steps

  // finalize precice interface
  this->preciceSolverInterface_->finalize();

#else
  LOG(FATAL) << "Not compiled with preCICE!";
#endif
}

#ifdef HAVE_PRECICE

#endif

template<typename NestedSolver>
void PreciceAdapter<NestedSolver>::
reset()
{
  this->nestedSolver_.reset();

  this->initialized_ = false;
  // "uninitialize" everything
}

template<typename NestedSolver>
typename PreciceAdapter<NestedSolver>::Data &PreciceAdapter<NestedSolver>::
data()
{
  // get a reference to the data object
  return this->nestedSolver_.data();
}

}  // namespace