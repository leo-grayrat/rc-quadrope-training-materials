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

}  // namespace

int main()
{
    Motor motor1;
    motor1.id = 7;
    motor1.position = -1.25;

    check(motor1.id == 7, "id can be stored");
    check(std::abs(motor1.position + 1.25) < 1e-9, "position can be stored");

    std::ostringstream output1;
    auto* old_buffer = std::cout.rdbuf(output1.rdbuf());
    motor1.printStatus();
    std::cout.rdbuf(old_buffer);

    check(output1.str() == "Motor 7 position: -1.25 rad\n",
          "printStatus formats a negative position");

    Motor motor2;
    motor2.id = 12;
    motor2.position = 0.5;

    std::ostringstream output2;
    old_buffer = std::cout.rdbuf(output2.rdbuf());
    motor2.printStatus();
    std::cout.rdbuf(old_buffer);

    check(output2.str() == "Motor 12 position: 0.50 rad\n",
          "printStatus keeps two decimal places");

    std::cout << passed << " / " << total << " tests passed" << std::endl;
    return passed == total ? 0 : 1;
}
