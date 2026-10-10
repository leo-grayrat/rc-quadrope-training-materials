#include "control_math.hpp"

#include <limits>

double pd_torque(
    double q,
    double dq,
    double q_des,
    double dq_des,
    double kp,
    double kd,
    double tau_ff,
    double tau_limit)
{
  (void)q;
  (void)dq;
  (void)q_des;
  (void)dq_des;
  (void)kp;
  (void)kd;
  (void)tau_ff;
  (void)tau_limit;
  return std::numeric_limits<double>::quiet_NaN();
}

Target linear_target(
    double q_start,
    double q_target,
    double elapsed,
    double duration)
{
  (void)q_start;
  (void)q_target;
  (void)elapsed;
  (void)duration;
  const double nan = std::numeric_limits<double>::quiet_NaN();
  return {nan, nan};
}
