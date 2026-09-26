#include "robot/unitree_motor.hpp"

#include <iomanip>
#include <iostream>

namespace robot {

UnitreeMotor::UnitreeMotor(int id)
    : id_(id), position_(0.0)
{
}

void UnitreeMotor::enable()
{
    // TODO: 输出 "Unitree Motor <id> enabled."
}

void UnitreeMotor::setPosition(double position)
{
    // TODO: 保存目标位置，并按两位小数输出：
    // Unitree Motor <id> -> target = <position> rad
}

double UnitreeMotor::getPosition() const
{
    // TODO: 返回当前保存的位置
    return 0.0;
}

}  // namespace robot
