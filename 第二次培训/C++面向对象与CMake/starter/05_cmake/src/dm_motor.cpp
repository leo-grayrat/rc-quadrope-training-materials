#include "robot/dm_motor.hpp"

#include <iomanip>
#include <iostream>

namespace robot {

DMMotor::DMMotor(int id)
    : id_(id), position_(0.0)
{
}

void DMMotor::enable()
{
    // TODO: 输出 "DM Motor <id> enabled."
}

void DMMotor::setPosition(double position)
{
    // TODO: 保存目标位置，并按两位小数输出：
    // DM Motor <id> -> target = <position> rad
}

double DMMotor::getPosition() const
{
    // TODO: 返回当前保存的位置
    return 0.0;
}

}  // namespace robot
