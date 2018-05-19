#pragma once

#include <Python.h>  // has to be the first included header
#include "time_stepping_scheme/time_stepping_scheme_ode.h"
#include "control/runnable.h"
#include "data_management/time_stepping.h"
#include "control/dihu_context.h"
#include "model_order_reduction/pod.h"

namespace TimeSteppingScheme
{

/** A specialized solver for the multidomain equation, as formulated by Thomas Klotz (2017)
  */
template<typename DiscretizableInTime>
class MultidomainSolver :
  public TimeSteppingSchemeOde<DiscretizableInTime>, public Runnable
{
public:

  //! constructor
  MultidomainSolver(DihuContext context);

  //! advance simulation by the given time span, data in solution is used, afterwards new data is in solution
  void advanceTimeSpan();

  //! run the simulation
  void run();
private:
};

}  // namespace

#include "time_stepping_scheme/multidomain_solver.tpp"