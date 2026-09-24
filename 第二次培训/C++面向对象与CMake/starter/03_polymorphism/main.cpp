#include <iomanip>
#include <iostream>

class Motor {
public:
    virtual void enable() = 0;
    virtual void setPosition(double position) = 0;
    virtual double getPosition() const = 0;
    virtual ~Motor() = default;
};

// TODO: 实现 DMMotor。
// 保存自己的 id 和 position，position 初始为 0.0。
// 实现 Motor 规定的三个接口。

// TODO: 实现 UnitreeMotor。
// 要求与 DMMotor 相同，但输出品牌名不同。

void controlMotor(Motor& motor)
{
    motor.enable();
    motor.setPosition(1.50);
}

int main()
{
    DMMotor dm_motor(1);
    UnitreeMotor unitree_motor(2);

    controlMotor(dm_motor);
    controlMotor(unitree_motor);

    return 0;
}
