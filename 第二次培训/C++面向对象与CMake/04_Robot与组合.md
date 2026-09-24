# Robot 与组合

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

```cpp
class Robot {
private:
    DMMotor left_motor_;
    DMMotor right_motor_;
};
```

那么这个 `Robot` 从类型上已经绑定 DM 电机。以后右腿想换 Unitree，就必须修改 `Robot`。

如果 `Robot` 只依赖 `Motor` 接口：

```cpp
class Robot {
private:
    Motor& left_motor_;
    Motor& right_motor_;
};
```

那么左右两边可以是任何满足接口的实现。

### 5.2 引用成员与生命周期

引用成员必须在构造时绑定，因此构造函数需要初始化列表：

```cpp
Robot(Motor& left_motor, Motor& right_motor)
    : left_motor_(left_motor),
      right_motor_(right_motor)
{
}
```

这里的 `Robot` **不拥有**这两台电机，它只是引用外部已经存在的对象。因此必须保证：

> `Robot` 使用这些引用期间，被引用的电机对象仍然存在。

下面的顺序是安全的：

```cpp
DMMotor left(1);
UnitreeMotor right(2);
Robot robot(left, right);
```

因为 `robot` 会先析构，然后才轮到在它之前创建的 `right` 和 `left`。

在更复杂项目中，“谁拥有对象、谁只借用对象、对象活多久”会成为很重要的问题。本文暂时不引入 `std::unique_ptr` / `std::shared_ptr`，但至少要知道引用并不会自动延长对象生命周期。

### 5.3 第四次动手：实现 Robot

假设前一节的 `Motor`、`DMMotor`、`UnitreeMotor` 已经写好。

补全下面的 `Robot`：

起始代码放在：

- [starter/04_robot/main.cpp](./starter/04_robot/main.cpp)

前一节的电机接口与两种具体电机已经给定，这次只补 `Robot` 中的 TODO。

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
