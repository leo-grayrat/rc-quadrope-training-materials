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

    left.setPosition(-0.25);
    right.setPosition(0.75);
    robot.printStatus();

    std::cout.rdbuf(old_buffer);

    check(left.enable_count == 1, "initialize enables left motor once");
    check(right.enable_count == 1, "initialize enables right motor once");
    check(left.set_count == 2 && right.set_count == 2,
          "Robot move calls each motor exactly once");

    check(close(left.getPosition(), -0.25), "left motor state remains independent");
    check(close(right.getPosition(), 0.75), "right motor state remains independent");

    check(output.str() ==
              "Robot initialization...\n"
              "Set robot target position: -0.80 rad\n"
              "Left motor position: -0.25 rad\n"
              "Right motor position: 0.75 rad\n",
          "Robot output and status reading");

    std::cout << passed << " / " << total << " tests passed" << std::endl;
    return passed == total ? 0 : 1;
}
