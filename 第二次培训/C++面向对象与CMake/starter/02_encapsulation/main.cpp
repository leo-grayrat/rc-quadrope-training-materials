#include <iomanip>
#include <iostream>

class Motor {
private:
    int id_;
    double position_;

public:
    // TODO: 构造函数
    // 创建 Motor(id) 时保存编号，position 初始为 0.0。

    // TODO: bool setPosition(double position)
    // 只有 [-3.14, 3.14] 内的位置才接受。

    // TODO: double getPosition() const

    // TODO: void printStatus() const
};

int main()
{
    Motor motor(3);

    std::cout << std::boolalpha;
    std::cout << motor.setPosition(1.25) << std::endl;
    motor.printStatus();

    std::cout << motor.setPosition(4.00) << std::endl;
    motor.printStatus();

    return 0;
}
