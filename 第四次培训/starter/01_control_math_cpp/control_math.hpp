#pragma once

struct Target
{
  double q_des;
  double dq_des;
};

double pd_torque(
    double q,
    double dq,
    double q_des,
    double dq_des,
    double kp,
    double kd,
    double tau_ff,
    double tau_limit);

Target linear_target(
    double q_start,
    double q_target,
    double elapsed,
    double duration);
