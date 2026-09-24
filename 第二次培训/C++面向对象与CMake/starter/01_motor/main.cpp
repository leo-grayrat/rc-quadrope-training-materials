#include <iomanip>
#include <iostream>

class Motor {
    // TODO:
    // 保存整数编号 id 和当前位置 position，
    // 并实现 printStatus()。
};

int main()
{
    Motor motor1;
    motor1.id = 1;
    motor1.position = 0.0;

    Motor motor2;
    motor2.id = 2;
    motor2.position = 0.5;

    motor1.printStatus();
    motor2.printStatus();

    return 0;
}
