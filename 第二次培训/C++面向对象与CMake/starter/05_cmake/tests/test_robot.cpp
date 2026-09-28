#include "robot/dm_motor.hpp"
#include "robot/robot.hpp"
#include "robot/unitree_motor.hpp"

#include <cmath>
#include <iostream>
#include <sstream>
#include <string>

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

class TestMotor : public robot::Motor {
public:
    void enable() override
    {
        ++enable_count;
    }

    void setPosition(double position) override
    {
        ++set_count;
        position_ = position;
    }

    double getPosition() const override
    {
        return position_;
    }

    void forcePosition(double position)
    {
        position_ = position;
    }

    int enable_count = 0;
    int set_count = 0;

private:
    double position_ = 0.0;
};

}  // namespace

int main()
{
    robot::DMMotor dm(5);
    robot::UnitreeMotor unitree(6);

    check(close(dm.getPosition(), 0.0), "DM motor starts at 0.0");
    check(close(unitree.getPosition(), 0.0), "Unitree motor starts at 0.0");

    std::ostringstream motor_output;
    auto* old_buffer = std::cout.rdbuf(motor_output.rdbuf());

    dm.enable();
    dm.setPosition(-1.20);
    unitree.enable();
    unitree.setPosition(2.30);

    std::cout.rdbuf(old_buffer);

    check(close(dm.getPosition(), -1.20), "DM setPosition stores the target");
    check(close(unitree.getPosition(), 2.30), "Unitree setPosition stores the target");

    check(motor_output.str() ==
              "DM Motor 5 enabled.\n"
              "DM Motor 5 -> target = -1.20 rad\n"
              "Unitree Motor 6 enabled.\n"
              "Unitree Motor 6 -> target = 2.30 rad\n",
          "motor output format");

    TestMotor left;
    TestMotor right;
    robot::Robot robot(left, right);

    std::ostringstream robot_output;
    old_buffer = std::cout.rdbuf(robot_output.rdbuf());

    robot.initialize();
    robot.move(0.60);

    std::cout.rdbuf(old_buffer);

    check(left.enable_count == 1 && right.enable_count == 1,
          "Robot initialize enables both motors once");
    check(left.set_count == 1 && right.set_count == 1,
          "Robot move sends one target to each motor");
    check(close(left.getPosition(), 0.60) && close(right.getPosition(), 0.60),
          "Robot move sends the same target to both motors");

    left.forcePosition(-0.20);
    right.forcePosition(0.40);

    std::ostringstream status_output;
    old_buffer = std::cout.rdbuf(status_output.rdbuf());
    robot.printStatus();
    std::cout.rdbuf(old_buffer);

    check(robot_output.str() ==
              "Robot initialization...\n"
              "Set robot target position: 0.60 rad\n",
          "Robot initialize and move output");

    check(status_output.str() ==
              "Left motor position: -0.20 rad\n"
              "Right motor position: 0.40 rad\n",
          "Robot printStatus reads both motor states");

    std::cout << passed << " / " << total << " tests passed" << std::endl;
    return passed == total ? 0 : 1;
}
