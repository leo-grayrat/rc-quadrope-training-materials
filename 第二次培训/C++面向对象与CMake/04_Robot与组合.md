## 5. Robot 与组合：对象之间怎样形成更大的系统

继承表达的是：

```text
DMMotor is a Motor
```

但机器人与电机之间不是这种关系。机器人不是一种电机，而是**使用电机**：

```text
Robot has Motors
```

真实系统中还可能有：

```text
Robot
├── Motors
├── IMU
├── Controller
├── Camera
└── ...
```

这类关系通常通过成员对象、引用、指针等方式表达，统称为组合关系。本文仍然只使用左右两台电机，以免同时引入容器、资源所有权等额外问题。

### 5.1 为什么 Robot 不直接写 DMMotor

如果写：

起始代码已经单独放在：

- [starter/04_robot/main.cpp](./starter/04_robot/main.cpp)

前一节的电机接口与两种电机实现已经给好，只补 `Robot`。

测试代码：

```cpp
int main()
{
    DMMotor left_motor(1);
    UnitreeMotor right_motor(2);
    Robot robot(left_motor, right_motor);

    robot.initialize();
    robot.move(1.50);
    robot.printStatus();

    return 0;
}
```

`Robot` 自己需要额外输出：

```text
Robot initialization...
Set robot target position: 1.50 rad
Left motor position: 1.50 rad
Right motor position: 1.50 rad
```

结合电机自身输出，完整结果应为：

```text
Robot initialization...
DM Motor 1 enabled.
Unitree Motor 2 enabled.
Set robot target position: 1.50 rad
DM Motor 1 -> target = 1.50 rad
Unitree Motor 2 -> target = 1.50 rad
Left motor position: 1.50 rad
Right motor position: 1.50 rad
```

<details>
<summary>参考实现</summary>

```cpp
class Robot {
public:
    Robot(Motor& left_motor, Motor& right_motor)
        : left_motor_(left_motor),
          right_motor_(right_motor)
    {
    }

    void initialize()
    {
        std::cout << "Robot initialization..." << std::endl;
        left_motor_.enable();
        right_motor_.enable();
    }

    void move(double position)
    {
        std::cout << "Set robot target position: "
                  << std::fixed << std::setprecision(2)
                  << position << " rad" << std::endl;

        left_motor_.setPosition(position);
        right_motor_.setPosition(position);
    }

    void printStatus() const
    {
        std::cout << "Left motor position: "
                  << std::fixed << std::setprecision(2)
                  << left_motor_.getPosition() << " rad" << std::endl;

        std::cout << "Right motor position: "
                  << std::fixed << std::setprecision(2)
                  << right_motor_.getPosition() << " rad" << std::endl;
    }

private:
    Motor& left_motor_;
    Motor& right_motor_;
};
```

</details>

到这里为止，我们仍然可以把所有类放在一个 `main.cpp`。这适合学习概念，却不适合继续扩展。下一步开始进入真正的工程组织。

---
