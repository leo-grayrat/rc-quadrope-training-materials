#define main student_main
#include "../main.cpp"
#undef main

#include <cmath>
#include <iostream>
#include <sstream>
#include <string>
#include <type_traits>
#include <utility>

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

template <typename T, typename = void>
struct has_public_id : std::false_type {
};

template <typename T>
struct has_public_id<T, std::void_t<decltype(std::declval<T&>().id)>>
    : std::true_type {
};

template <typename T, typename = void>
struct has_public_position : std::false_type {
};

template <typename T>
struct has_public_position<T, std::void_t<decltype(std::declval<T&>().position)>>
    : std::true_type {
};

}  // namespace

int main()
{
    Motor motor(9);

    check(close(motor.getPosition(), 0.0), "initial position is 0.0");
    check(!has_public_id<Motor>::value, "id is not publicly writable");
    check(!has_public_position<Motor>::value, "position is not publicly writable");

    check(motor.setPosition(3.14), "upper boundary is accepted");
    check(close(motor.getPosition(), 3.14), "upper boundary is stored");

    check(!motor.setPosition(3.1401), "value above upper boundary is rejected");
    check(close(motor.getPosition(), 3.14), "rejected value keeps old position");

    check(motor.setPosition(-3.14), "lower boundary is accepted");
    check(close(motor.getPosition(), -3.14), "lower boundary is stored");

    check(!motor.setPosition(-3.1401), "value below lower boundary is rejected");
    check(close(motor.getPosition(), -3.14), "second rejected value keeps old position");

    std::ostringstream output;
    auto* old_buffer = std::cout.rdbuf(output.rdbuf());
    motor.printStatus();
    std::cout.rdbuf(old_buffer);

    check(output.str() == "Motor 9 position: -3.14 rad\n",
          "printStatus uses the stored value");

    std::cout << passed << " / " << total << " tests passed" << std::endl;
    return passed == total ? 0 : 1;
}
