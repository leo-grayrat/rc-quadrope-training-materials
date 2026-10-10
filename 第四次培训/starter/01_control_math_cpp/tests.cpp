#include "control_math.hpp"

#include <cmath>
#include <iostream>
#include <string>

namespace
{
int passed = 0;
int total = 0;

void expect_near(const std::string & name, double actual, double expected, double eps = 1e-9)
{
  ++total;
  if (std::isfinite(actual) && std::abs(actual - expected) <= eps) {
    ++passed;
    std::cout << "[PASS] " << name << '\n';
  } else {
    std::cout << "[FAIL] " << name << ": expected " << expected
              << ", got " << actual << '\n';
  }
}
}  // namespace

int main()
{
  expect_near(
      "pd ordinary",
      pd_torque(0.5, 0.1, 0.0, 0.0, 20.0, 1.0, 0.0, 100.0),
      -10.1);

  expect_near(
      "pd damping",
      pd_torque(1.7, 3.0, -2.4, 0.0, 0.0, 2.0, 0.0, 100.0),
      -6.0);

  expect_near(
      "pd positive saturation",
      pd_torque(0.0, 0.0, 10.0, 0.0, 100.0, 0.0, 0.0, 5.0),
      5.0);

  expect_near(
      "pd negative saturation",
      pd_torque(10.0, 0.0, 0.0, 0.0, 100.0, 0.0, 0.0, 5.0),
      -5.0);

  const Target start = linear_target(0.0, 1.0, 0.0, 2.0);
  expect_near("trajectory start position", start.q_des, 0.0);
  expect_near("trajectory start velocity", start.dq_des, 0.5);

  const Target middle = linear_target(0.0, 1.0, 1.0, 2.0);
  expect_near("trajectory middle position", middle.q_des, 0.5);
  expect_near("trajectory middle velocity", middle.dq_des, 0.5);

  const Target end = linear_target(0.0, 1.0, 2.0, 2.0);
  expect_near("trajectory end position", end.q_des, 1.0);
  expect_near("trajectory end velocity", end.dq_des, 0.0);

  const Target after = linear_target(0.0, 1.0, 3.0, 2.0);
  expect_near("trajectory after end position", after.q_des, 1.0);
  expect_near("trajectory after end velocity", after.dq_des, 0.0);

  const Target reverse = linear_target(1.0, -1.0, 1.0, 2.0);
  expect_near("trajectory reverse position", reverse.q_des, 0.0);
  expect_near("trajectory reverse velocity", reverse.dq_des, -1.0);

  std::cout << passed << " / " << total << " checks passed\n";
  return passed == total ? 0 : 1;
}
