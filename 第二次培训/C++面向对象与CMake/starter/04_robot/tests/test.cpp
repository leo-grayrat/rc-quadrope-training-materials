#define main student_main
#include "../main.cpp"
#undef main

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

class TestMotor : public Motor {
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
    TestMotor left;
    TestMotor right;
    Robot robot(left, right);

    std::ostringstream output;
    auto* old_buffer = std::cout.rdbuf(output.rdbuf());

    robot.initialize();
    robot.move(-0.80);

    std::cout.rdbuf(old_buffer);

    check(left.enable_count == 1, "initialize enables left motor once");
    check(right.enable_count == 1, "initialize enables right motor once");
    check(left.set_count == 1 && right.set_count == 1,
          "Robot move calls each motor exactly once");
    check(close(left.getPosition(), -0.80) && close(right.getPosition(), -0.80),
          "Robot move sends the same target to both motors");

    left.forcePosition(-0.25);
    right.forcePosition(0.75);

    std::ostringstream status_output;
    old_buffer = std::cout.rdbuf(status_output.rdbuf());
    robot.printStatus();
    std::cout.rdbuf(old_buffer);

    check(status_output.str() ==
              "Left motor position: -0.25 rad\n"
              "Right motor position: 0.75 rad\n",
          "printStatus reads two independent motor states");

    check(output.str() ==
              "Robot initialization...\n"
              "Set robot target position: -0.80 rad\n",
          "initialize and move output");

    std::cout << passed << " / " << total << " tests passed" << std::endl;
    return passed == total ? 0 : 1;
}
