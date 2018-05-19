#pragma once

#include "time_stepping_scheme/time_stepping_scheme_ode.h"
#include "control/runnable.h"
#include "data_management/time_stepping_heun.h"
#include "control/dihu_context.h"
#include "model_order_reduction/pod.h"

namespace TimeSteppingScheme
{

/** The Heun integration scheme, u_{t+1} = u_{t} + 0.5*dt*(f(u_{t})+f(u*)) where u* = u_{t} + dt*f(u_{t}).
 *
 *  However, we compute it in the way: u_{t+1} = u* + 0.5*dt*(f(u*)-f(u_{t}))
 *  (more round off this way, but less storage required)
 *
 * Blödsinn: f_t - f* ist gar nicht möglich ohne zusätzlichen vektor!
  */
template<typename DiscretizableInTime>
class Heun :
  public TimeSteppingSchemeOde<DiscretizableInTime>, public Runnable
{
public:

  //! constructor
  Heun(DihuContext context);

  //! advance simulation by the given time span [startTime_, endTime_] with given numberTimeSteps, data in solution is used, afterwards new data is in solution
  void advanceTimeSpan();

  //! run the simulation
  void run();
private:
};

}  // namespace

#include "time_stepping_scheme/heun.tpp"