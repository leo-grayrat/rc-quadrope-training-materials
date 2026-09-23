#include "robot/robot.hpp"

#include <iomanip>
#include <iostream>

namespace robot {

Robot::Robot(Motor& left_motor, Motor& right_motor)
    : left_motor_(left_motor), right_motor_(right_motor)
{
}

void Robot::initialize()
{
    // TODO:
    // 1. 输出 "Robot initialization..."
    // 2. 依次 enable 左右电机
}

void Robot::move(double position)
{
    // TODO:
    // 1. 输出 "Set robot target position: <position> rad"
    // 2. 把同一个目标位置发送给左右电机
}

void Robot::printStatus() const
{
    // TODO:
    // 按两位小数输出：
    // Left motor position: <position> rad
    // Right motor position: <position> rad
}

}  // namespace robot
