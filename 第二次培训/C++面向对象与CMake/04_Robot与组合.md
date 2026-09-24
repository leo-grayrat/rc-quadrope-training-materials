# Robot 与组合

上一节建立了统一的 `Motor` 接口。接下来把两台电机放进一个 `Robot` 对象中。

`DMMotor` 和 `Motor` 是继承关系：`DMMotor` 属于 `Motor` 的一种具体实现。

`Robot` 和 `Motor` 的关系不同。机器人内部使用电机，还可能使用 IMU、控制器、相机等部件：

```text
Robot
├── Motors
├── IMU
├── Controller
├── Camera
└── ...
```

这种“一个对象内部持有或使用其他对象”的结构通常称为组合。

## Robot 依赖 Motor 接口

如果 `Robot` 直接保存具体的 DM 电机：

```cpp
class Robot {
private:
    DMMotor left_motor_;
    DMMotor right_motor_;
};
```

这样 `Robot` 的定义就和 `DMMotor` 绑定。以后如果一侧改用 Unitree 电机，`Robot` 也要修改。

改成保存 `Motor` 引用：

```cpp
class Robot {
private:
    Motor& left_motor_;
    Motor& right_motor_;
};
```

以后左右两侧可以分别传入 `DMMotor`、`UnitreeMotor`，或者其他实现了 `Motor` 接口的类型。

这样，`Robot` 只需要知道电机具有 `enable()`、`setPosition()`、`getPosition()` 这些操作。

## 引用成员与生命周期

引用成员必须在构造对象时完成绑定，所以构造函数需要使用初始化列表：

```cpp
Robot(Motor& left_motor, Motor& right_motor)
    : left_motor_(left_motor),
      right_motor_(right_motor)
{
}
```

这里的两个引用都指向外部已经存在的电机对象。创建顺序可以写成：

```cpp
DMMotor left(1);
UnitreeMotor right(2);
Robot robot(left, right);
```

三个对象都位于同一作用域时，析构顺序与创建顺序相反：先析构 `robot`，再析构 `right` 和 `left`。因此 `robot` 存在期间，它引用的两台电机也仍然存在。

引用不会延长对象生命周期。以后如果对象由其他模块创建、动态创建，或者生命周期更复杂，就需要进一步考虑所有权和智能指针。

## 动手：实现 Robot

起始代码：

- [starter/04_robot/main.cpp](./starter/04_robot/main.cpp)

`Motor`、`DMMotor`、`UnitreeMotor` 已经给好，只实现 `Robot` 中的三个函数：

- `initialize()`：输出初始化信息，然后依次使能左右电机；
- `move(position)`：向左右电机发送相同目标位置；
- `printStatus()`：打印左右电机当前位置。

测试入口已经写在 starter 的 `main()` 中。

完整输出应为：

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

到这里，程序已经有多个类。下一节开始把这些类拆到不同文件中。
