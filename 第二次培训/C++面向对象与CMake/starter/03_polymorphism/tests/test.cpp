#define main student_main
#include "../main.cpp"
#undef main

#include <cmath>
#include <iostream>
#include <sstream>
#include <string>
#include <type_traits>

namespace {

int passed = 0;
int total = 0;

void check(bool condition, const std::string& name)
{
    ++total;
    if (condition) {
        ++passed;
        std::cout << "[PASS] " << name << std::endl;
    } else {
        std::cout << "[FAIL] " << name << std::endl;
    }
}

bool close(double a, double b)
{
    return std::abs(a - b) < 1e-9;
}

}  // namespace

int main()
{
    check(std::is_abstract<Motor>::value, "Motor remains abstract");
    check(std::is_base_of<Motor, DMMotor>::value, "DMMotor inherits Motor");
    check(std::is_base_of<Motor, UnitreeMotor>::value, "UnitreeMotor inherits Motor");

    DMMotor dm(7);
    UnitreeMotor unitree(8);

    check(close(dm.getPosition(), 0.0), "DM motor starts at 0.0");
    check(close(unitree.getPosition(), 0.0), "Unitree motor starts at 0.0");

    Motor& dm_as_motor = dm;
    Motor& unitree_as_motor = unitree;

    std::ostringstream dm_output;
    auto* old_buffer = std::cout.rdbuf(dm_output.rdbuf());
    dm_as_motor.enable();
    dm_as_motor.setPosition(-0.80);
    std::cout.rdbuf(old_buffer);

    check(close(dm.getPosition(), -0.80),
          "DM state changes through Motor reference");
    check(dm_output.str() ==
              "DM Motor 7 enabled.\n"
              "DM Motor 7 -> target = -0.80 rad\n",
          "DM output format");

    std::ostringstream unitree_output;
    old_buffer = std::cout.rdbuf(unitree_output.rdbuf());
    unitree_as_motor.enable();
    unitree_as_motor.setPosition(2.25);
    std::cout.rdbuf(old_buffer);

    check(close(unitree.getPosition(), 2.25),
          "Unitree state changes through Motor reference");
    check(unitree_output.str() ==
              "Unitree Motor 8 enabled.\n"
              "Unitree Motor 8 -> target = 2.25 rad\n",
          "Unitree output format");

    std::cout << passed << " / " << total << " tests passed" << std::endl;
    return passed == total ? 0 : 1;
}
