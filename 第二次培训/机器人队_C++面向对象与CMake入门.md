# 机器人队 C++ 面向对象与 CMake 入门（DIY 版）

本文面向四足组后续机器人软件开发所需要的 C++ 面向对象与基础工程能力。内容仍然沿用原讲义的主线：从一个简单的 `Motor` 类开始，逐步引入封装、构造函数、抽象接口、继承与多态，再把电机组合进 `Robot`，最后把单文件程序拆成多文件工程并使用 CMake 构建。

与直接展示完整代码不同，本文会在关键位置停下来给出**明确的任务、起始代码和可验证的运行结果**。建议第一次阅读时不要立即展开参考实现，而是先完成 TODO。知识讲解仍然是正文主体；动手部分只保留在真正需要把新概念写进代码的地方，不会把每一个知识点都拆成一道独立题目。

> 本文中的电机均为终端模拟对象，不会向真实电机发送指令。位置、使能等接口用于说明软件结构，不代表真实驱动协议。

---

## 1. 从一个最小 C++ 程序开始

新建一个目录：

```bash
mkdir -p ~/robot_cpp_training/oop_cmake
cd ~/robot_cpp_training/oop_cmake
```

创建 `hello.cpp`：

```cpp
#include <iostream>

int main()
{
    std::cout << "Hello Robot Team!" << std::endl;
    return 0;
}
```

编译并运行：

```bash
g++ hello.cpp -std=c++17 -o hello
./hello
```

这里发生了两件事：

1. `g++` 将 C++ 源代码编译、链接成可执行文件 `hello`；
2. Shell 再执行当前目录中的 `hello`。

如果看到：

```text
./hello: No such file or directory
```

不要马上把它理解成“代码写错了”。先执行：

```bash
pwd
ls
```

确认自己是否在正确目录、编译是否真的成功。以后做机器人项目时，很多问题同样来自“文件在哪、当前目录在哪、实际编译的是哪份代码”，而不只是 C++ 语法本身。

本文始终写 `std::cout`、`std::string` 等完整名称，不使用 `using namespace std;`。在小程序里后者很方便，但工程中不同库可能定义同名符号，从一开始保留命名空间会更清楚。

---

## 2. 类与对象：先把“一台电机”组织起来

### 2.1 为什么需要类

假设程序只需要处理两台电机，可以直接写：

```cpp
int motor1_id = 1;
double motor1_position = 0.0;

int motor2_id = 2;
double motor2_position = 0.5;
```

如果接着增加速度、力矩、温度，变量会迅速变成：

```text
motor1_id
motor1_position
motor1_velocity
motor1_torque
motor1_temperature
motor2_id
motor2_position
...
```

四足机器人往往有十几个关节。此时真正的问题不是“变量太多”，而是**属于同一台电机的数据没有形成一个整体**。

C++ 的 `class` 可以定义一种新的类型：

```cpp
class Motor {
    // 描述一台 Motor 对象具有哪些数据和操作
};
```

类是类型的定义，对象是这个类型的具体实例：

```cpp
Motor motor1;
Motor motor2;
```

`motor1` 和 `motor2` 是两个不同对象，它们各自拥有自己的成员变量。

类中的变量称为**成员变量**，类中的函数称为**成员函数**。成员函数可以直接访问当前对象的成员变量，因此“数据”和“围绕这些数据进行的操作”可以放在同一个地方。

### 2.2 第一次动手：补全最基础的 Motor

这一小节只练习：类、对象、成员变量和成员函数。

**任务要求**

实现一个 `Motor` 类。每个对象保存：

- 整数编号 `id`；
- 当前位置 `position`，单位为 rad。

目前这两个成员可以先公开。类还需要提供 `printStatus()`，按下面格式输出状态：

```text
Motor 1 position: 0.00 rad
```

下面的 `main()` 已经给定，不修改它。只补全 `Motor`。

```cpp
#include <iomanip>
#include <iostream>

class Motor {
    // TODO
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
```

预期输出：

```text
Motor 1 position: 0.00 rad
Motor 2 position: 0.50 rad
```

这里已经把“程序要做什么”规定清楚，但没有规定成员变量必须写成什么排版、成员函数内部具体怎样组织输出。先自己完成。

<details>
<summary>参考实现</summary>

```cpp
class Motor {
public:
    int id;
    double position;

    void printStatus() const
    {
        std::cout << "Motor " << id << " position: "
                  << std::fixed << std::setprecision(2)
                  << position << " rad" << std::endl;
    }
};
```

`std::fixed` 与 `std::setprecision(2)` 用来固定显示两位小数。`printStatus()` 后面的 `const` 表示这个成员函数不会修改当前对象；下一节会更系统地解释。

</details>

---

## 3. 封装、构造函数与对象的有效状态

上一节的代码可以运行，但有一个明显问题：

```cpp
motor1.position = 100000.0;
```

类外任何代码都可以直接写入任意值。对于“只保存几个演示数据”这不一定立刻出错，但一旦我们希望保证某些规则成立，例如：

```text
-3.14 <= position <= 3.14
```

公开成员变量就会绕过所有检查。

### 3.1 public 与 private

类中常用两种访问权限：

```cpp
class Motor {
public:
    // 类外可以访问

private:
    // 只有类自己的成员函数可以访问
};
```

把状态放入 `private` 并不是一种固定仪式。它的价值在于：**外部只能通过类提供的操作改变状态，因此类有机会维护自己的规则。**

例如：

```cpp
motor.setPosition(1.0);
```

以后即使需要限位、记录错误、检查使能状态，调用者仍然可以使用同一个接口。

### 3.2 构造函数

如果写：

```cpp
Motor motor;
```

然后再分别设置 `id` 和 `position`，对象会经历一个“已经存在但还没有初始化完整”的阶段。更自然的方式是让对象在创建时直接进入有效状态。

构造函数与类同名，没有返回类型：

```cpp
Motor(int id)
{
    // 初始化
}
```

更常见的写法是成员初始化列表：

```cpp
Motor(int id)
    : id_(id), position_(0.0)
{
}
```

这里不是“先创建成员，再在函数体里赋值”，而是在构造成员时就用给定值初始化它们。对于引用成员、`const` 成员以及很多复杂对象，初始化列表尤其重要。

成员实际按照**在类中声明的顺序**初始化，而不是按照初始化列表书写顺序。为了避免误读，通常让两者顺序一致。

### 3.3 const 成员函数

如果一个函数只读取对象，不应修改对象状态，可以写：

```cpp
double getPosition() const;
```

`const` 在这里约束的是当前对象。编译器会阻止这个函数直接修改普通成员变量。

因此：

```cpp
double getPosition() const
{
    return position_;
}
```

是合理的；而在其中写：

```cpp
position_ = 1.0;
```

会产生编译错误。

### 3.4 explicit

只有一个参数的构造函数有时会参与隐式类型转换：

```cpp
Motor m = 3;
```

对于电机编号这样的场景，这种写法通常没有必要，甚至会让错误代码悄悄通过。因此可以写：

```cpp
explicit Motor(int id);
```

它要求调用者明确写：

```cpp
Motor m(3);
```

### 3.5 第二次动手：让 Motor 自己维护位置范围

现在把上一节的 `Motor` 改造成真正封装状态的版本。

**任务要求**

- 创建 `Motor(id)` 时保存编号，位置初始化为 `0.0`；
- `id` 与 `position` 不能从类外直接修改；
- `setPosition(x)` 仅在 `x` 位于 `[-3.14, 3.14]` 时更新位置；
- 合法时返回 `true`，非法时保持原位置并返回 `false`；
- `getPosition()` 返回当前位置；
- `printStatus()` 保持上一节的输出格式。

起始代码：

```cpp
#include <iomanip>
#include <iostream>

class Motor {
private:
    int id_;
    double position_;

public:
    // TODO: 构造函数

    // TODO: setPosition

    // TODO: getPosition

    // TODO: printStatus
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
```

预期输出：

```text
true
Motor 3 position: 1.25 rad
false
Motor 3 position: 1.25 rad
```

第二次设置失败后，位置必须仍然是 `1.25`。

<details>
<summary>参考实现</summary>

```cpp
class Motor {
private:
    int id_;
    double position_;

public:
    explicit Motor(int id)
        : id_(id), position_(0.0)
    {
    }

    bool setPosition(double position)
    {
        if (position < -3.14 || position > 3.14) {
            return false;
        }

        position_ = position;
        return true;
    }

    double getPosition() const
    {
        return position_;
    }

    void printStatus() const
    {
        std::cout << "Motor " << id_ << " position: "
                  << std::fixed << std::setprecision(2)
                  << position_ << " rad" << std::endl;
    }
};
```

这个版本的关键不是“getter/setter 的格式”，而是类已经能够保证：只要外部不破坏语言规则，`position_` 不会通过公开接口进入规定范围之外。

</details>

---

## 4. 抽象接口、继承与多态：让上层代码不关心电机品牌

前面的 `Motor` 只有一种实现。但实际机器人可能同时使用不同驱动方式的电机。

假设 DM 电机与 Unitree 电机底层通信完全不同，上层控制代码仍然希望表达同样的三件事：

```text
enable()
setPosition(position)
getPosition()
```

如果上层到处判断具体型号：

```cpp
if (type == DM) {
    ...
} else if (type == UNITREE) {
    ...
}
```

那么每增加一种电机都需要修改大量控制逻辑。

### 4.1 抽象类与纯虚函数

我们可以先定义“一个可被机器人使用的电机至少要提供什么能力”：

```cpp
class Motor {
public:
    virtual void enable() = 0;
    virtual void setPosition(double position) = 0;
    virtual double getPosition() const = 0;
    virtual ~Motor() = default;
};
```

带 `= 0` 的虚函数称为**纯虚函数**。包含纯虚函数的类不能直接创建对象：

```cpp
Motor motor;  // 不允许
```

这里的 `Motor` 更像一份接口约定：所有具体电机只要想被当作 `Motor` 使用，就必须实现这些能力。

`virtual ~Motor() = default;` 是虚析构函数。只要一个基类准备被多态使用，就应该认真考虑析构问题。本文没有动态分配对象，因此暂时不会展开所有权和智能指针；保留虚析构可以让这个接口以后通过基类指针释放派生对象时行为正确。

### 4.2 继承与 override

具体电机可以继承这个接口：

```cpp
class DMMotor : public Motor {
public:
    void enable() override;
    void setPosition(double position) override;
    double getPosition() const override;
};
```

`public Motor` 表达的是“DMMotor 是一种 Motor”。

`override` 不是必须的装饰。它会让编译器检查：这个函数是否真的覆盖了基类虚函数。如果误写参数、返回类型或 `const`，编译器会直接指出，而不是悄悄创建一个完全不同的函数。

### 4.3 多态与引用

有了共同接口，就可以写：

```cpp
void controlMotor(Motor& motor)
{
    motor.enable();
    motor.setPosition(1.5);
}
```

然后：

```cpp
DMMotor dm(1);
UnitreeMotor unitree(2);

controlMotor(dm);
controlMotor(unitree);
```

同一个 `controlMotor` 不需要知道实际型号。调用 `motor.enable()` 时，程序会根据真实对象执行不同实现。这就是这里最直接的多态。

为什么参数写 `Motor&`？

- 引用让函数操作已有对象，不额外复制一台电机；
- 基类引用可以绑定到派生类对象；
- 虚函数调用会保留派生对象的动态行为。

本文此处先使用引用，不提前引入裸指针与动态内存管理。

### 4.4 第三次动手：实现两种具体电机

这一题**直接给出接口**。接口是本阶段的需求，不要求你猜。

```cpp
#include <iomanip>
#include <iostream>

class Motor {
public:
    virtual void enable() = 0;
    virtual void setPosition(double position) = 0;
    virtual double getPosition() const = 0;
    virtual ~Motor() = default;
};

// TODO: DMMotor
// TODO: UnitreeMotor

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
```

要求：

- 两个类都保存自己的 `id` 和 `position`，初始位置为 `0.0`；
- `enable()` 分别输出：
  - `DM Motor <id> enabled.`
  - `Unitree Motor <id> enabled.`
- `setPosition(x)` 保存位置并分别输出：
  - `DM Motor <id> -> target = <x> rad`
  - `Unitree Motor <id> -> target = <x> rad`
- 位置统一保留两位小数；
- `getPosition()` 返回当前保存的位置。

预期输出：

```text
DM Motor 1 enabled.
DM Motor 1 -> target = 1.50 rad
Unitree Motor 2 enabled.
Unitree Motor 2 -> target = 1.50 rad
```

<details>
<summary>参考实现</summary>

```cpp
class DMMotor : public Motor {
public:
    explicit DMMotor(int id)
        : id_(id), position_(0.0)
    {
    }

    void enable() override
    {
        std::cout << "DM Motor " << id_ << " enabled." << std::endl;
    }

    void setPosition(double position) override
    {
        position_ = position;
        std::cout << "DM Motor " << id_ << " -> target = "
                  << std::fixed << std::setprecision(2)
                  << position_ << " rad" << std::endl;
    }

    double getPosition() const override
    {
        return position_;
    }

private:
    int id_;
    double position_;
};

class UnitreeMotor : public Motor {
public:
    explicit UnitreeMotor(int id)
        : id_(id), position_(0.0)
    {
    }

    void enable() override
    {
        std::cout << "Unitree Motor " << id_ << " enabled." << std::endl;
    }

    void setPosition(double position) override
    {
        position_ = position;
        std::cout << "Unitree Motor " << id_ << " -> target = "
                  << std::fixed << std::setprecision(2)
                  << position_ << " rad" << std::endl;
    }

    double getPosition() const override
    {
        return position_;
    }

private:
    int id_;
    double position_;
};
```

</details>

---

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

```cpp
class Robot {
public:
    Robot(Motor& left_motor, Motor& right_motor)
        : left_motor_(left_motor),
          right_motor_(right_motor)
    {
    }

    // TODO: initialize()
    // 两台电机依次 enable

    // TODO: move(double position)
    // 向两台电机发送同一个目标位置

    // TODO: printStatus() const
    // 按要求打印左右电机当前位置

private:
    Motor& left_motor_;
    Motor& right_motor_;
};
```

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

## 6. 从一个 main.cpp 拆成多文件工程

### 6.1 声明与定义

考虑：

```cpp
class DMMotor : public Motor {
public:
    explicit DMMotor(int id);
    void enable() override;
};
```

这里告诉编译器：

> 存在这些函数，它们的类型是这样。

这叫**声明**。

真正的实现可以放在另一个 `.cpp`：

```cpp
DMMotor::DMMotor(int id)
    : id_(id), position_(0.0)
{
}

void DMMotor::enable()
{
    ...
}
```

这才是**定义**。

头文件通常保存需要被其他源文件看到的声明；`.cpp` 保存具体实现。并不是所有函数都必须拆开，也不是“头文件只能写声明”这种绝对规则，但把稳定接口与具体实现分离，是中型 C++ 工程最常见的组织方式之一。

### 6.2 头文件、#include 与 #pragma once

假设 `dm_motor.hpp` 中继承了 `Motor`：

```cpp
#include "robot/motor.hpp"
```

预处理器会把需要的声明带到当前编译单元。

头文件可能被多条 include 链重复包含，因此常见做法是：

```cpp
#pragma once
```

它让同一个头文件在一次编译中只处理一次。传统 include guard 也能完成相同目的。

### 6.3 每个 cpp 是独立编译单元

假设工程有：

```text
apps/main.cpp
src/dm_motor.cpp
src/unitree_motor.cpp
src/robot.cpp
```

编译器并不会因为 `main.cpp` include 了头文件，就自动找到所有 `.cpp` 实现。

可以分别生成目标文件：

```bash
g++ -c src/dm_motor.cpp -Iinclude -std=c++17 -o dm_motor.o
g++ -c src/unitree_motor.cpp -Iinclude -std=c++17 -o unitree_motor.o
g++ -c src/robot.cpp -Iinclude -std=c++17 -o robot.o
g++ -c apps/main.cpp -Iinclude -std=c++17 -o main.o
```

再链接：

```bash
g++ main.o dm_motor.o unitree_motor.o robot.o -o robot_demo
```

也可以在一个命令里让 `g++` 完成这两阶段：

```bash
g++ apps/main.cpp \
    src/dm_motor.cpp \
    src/unitree_motor.cpp \
    src/robot.cpp \
    -Iinclude -std=c++17 -o robot_demo
```

`-Iinclude` 表示把 `include/` 加入头文件搜索路径，所以代码可以写：

```cpp
#include "robot/motor.hpp"
```

### 6.4 编译错误与链接错误不是一回事

如果写错：

```cpp
left_motor_.enable(
```

编译器连这个 `.cpp` 都无法变成目标文件，这是**编译错误**。

如果头文件里的类已经声明：

```cpp
class DMMotor {
public:
    void enable();
};
```

调用也能通过编译，但最终链接时没有把提供定义的 `dm_motor.cpp` 加进去，常见错误会包含：

```text
undefined reference to ...
```

这说明“我知道这个函数长什么样，但在最终要合成程序时找不到它的实现”。

排错时先判断错误发生在编译还是链接阶段，通常比直接盯着最后一行错误有效。

### 6.5 namespace

最终工程把类型放入：

```cpp
namespace robot {
    ...
}
```

使用时：

```cpp
robot::DMMotor motor(1);
```

命名空间主要用于控制名字所在的作用域。以后不同库、不同模块都可能出现 `Motor`、`Controller`、`State` 等常见名称，命名空间可以避免它们全部挤在全局作用域。

---

## 7. CMake：把构建关系写成工程描述

手动编译四个 `.cpp` 还能接受。等工程变成二十个、五十个文件，再自己维护命令很容易漏文件或写错依赖。

CMake **不是编译器**。可以把流程理解为：

```text
CMakeLists.txt
      ↓
    CMake
      ↓
Makefile / Ninja 等构建规则
      ↓
  构建工具
      ↓
     g++
      ↓
目标文件、库、可执行程序
```

CMake 的重要作用，是把：

- 有哪些 target；
- target 由哪些源文件组成；
- 头文件从哪里查找；
- target 之间怎样依赖；

写进可以维护的工程描述。

### 7.1 最简单的可执行 target

```cmake
cmake_minimum_required(VERSION 3.16)
project(mini_robot)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(robot_demo
    apps/main.cpp
    src/dm_motor.cpp
    src/unitree_motor.cpp
    src/robot.cpp
)

target_include_directories(robot_demo PRIVATE include)
```

`add_executable` 定义一个可执行 target。`target_include_directories` 给这个 target 增加头文件搜索目录。

### 7.2 为什么还会有 library target

如果核心机器人代码以后不仅被一个 demo 使用，而是还要被测试程序、ROS 2 节点或其他工具复用，更自然的结构是：

```cmake
add_library(robot_core
    src/dm_motor.cpp
    src/unitree_motor.cpp
    src/robot.cpp
)

target_include_directories(robot_core PUBLIC include)

add_executable(robot_demo apps/main.cpp)
target_link_libraries(robot_demo PRIVATE robot_core)
```

这里：

- `robot_core` 是一组可复用的已编译代码；
- `robot_demo` 包含 `main()`，可以直接运行；
- `robot_demo` 链接 `robot_core`。

`PUBLIC` 与 `PRIVATE` 描述的是依赖是否向使用者传播。这里 `robot_core` 的公开头文件就在 `include/`，使用 `robot_core` 的 target 也需要知道这个 include 路径，因此写 `PUBLIC`。而 `robot_demo` 自己链接 `robot_core`，不需要把这层依赖继续传播给其他 target，因此写 `PRIVATE`。

现在不必背一整套 CMake 关键字。先抓住 target：**先问“我要构建什么”，再问“这个 target 需要哪些源文件和依赖”。**

### 7.3 out-of-source build

推荐：

```bash
cmake -S . -B build
cmake --build build
./build/robot_demo
```

构建产物集中在 `build/`，源码目录保持干净。需要完全重新配置时，也可以删除 `build/` 后重建，而不必手工寻找散落在源码目录里的中间文件。

---

## 8. 第五次动手：完成 mini_robot 多文件工程

仓库中提供：

```text
第二次培训/oop_cmake_starter/
```

这一次不再要求从空目录猜最终工程应该长什么样。目录、头文件接口和测试入口已经提供，你需要补齐实现与构建描述。

目标结构：

```text
oop_cmake_starter/
├── CMakeLists.txt
├── README.md
├── apps/
│   └── main.cpp
├── include/
│   └── robot/
│       ├── dm_motor.hpp
│       ├── motor.hpp
│       ├── robot.hpp
│       └── unitree_motor.hpp
└── src/
    ├── dm_motor.cpp
    ├── robot.cpp
    └── unitree_motor.cpp
```

### 8.1 需要完成什么

1. 完成三个 `src/*.cpp` 中的 TODO；
2. 完成 `CMakeLists.txt`；
3. 构建并运行 `robot_demo`；
4. 不修改公开接口来绕开任务；
5. 能解释编译单元、链接、include 路径和 CMake target 分别在做什么。

构建：

```bash
cd 第二次培训/oop_cmake_starter
cmake -S . -B build
cmake --build build
./build/robot_demo
```

预期输出：

```text
=== Mini Robot Demo ===
Robot initialization...
DM Motor 1 enabled.
Unitree Motor 2 enabled.
Set robot target position: 1.50 rad
DM Motor 1 -> target = 1.50 rad
Unitree Motor 2 -> target = 1.50 rad
Left motor position: 1.50 rad
Right motor position: 1.50 rad
```

### 8.2 完成以后再做一次现场修改

把：

```cpp
robot.move(1.50);
```

改成：

```cpp
robot.move(-0.80);
```

重新：

```bash
cmake --build build
./build/robot_demo
```

确认所有相关位置都变为 `-0.80 rad`。

随后尝试故意从 `CMakeLists.txt` 中漏掉一个实现文件，再构建，观察错误发生在什么阶段。恢复后重新构建。

这一步不是为了制造麻烦，而是让“编译”和“链接”不只停留在定义上。

---

## 9. 常见排错顺序

以后遇到问题，不建议直接随机修改代码。先按错误所在层次缩小范围。

### 找不到文件或程序

先看：

```bash
pwd
ls
tree
```

### 找不到头文件

例如：

```text
fatal error: robot/motor.hpp: No such file or directory
```

检查：

- include 的路径是否写对；
- `-Iinclude` 是否存在；
- CMake 的 `target_include_directories` 是否配置到正确 target；
- 自己是否在预期工程目录构建。

### 编译错误

看错误指出的第一个源代码位置，从该行及其前一行开始检查。后面大量报错可能只是第一个语法错误的连锁反应。

### undefined reference

优先检查：

- 函数是否只有声明、没有定义；
- 定义的函数签名是否和头文件完全一致；
- 对应 `.cpp` 是否真的参与构建；
- 可执行程序是否链接了需要的 library target。

### 修改代码后运行结果没有变化

确认：

- 保存的是当前工程中的文件；
- 执行过 `cmake --build build`；
- 运行的是 `./build/robot_demo` 而不是别处同名的旧文件。

---

## 10. 验收时应该真正掌握什么

这一阶段不要求背诵 C++ 标准，也不要求手写复杂模板。至少应能够在自己完成的代码上解释：

- 类和对象的区别；
- 成员变量、成员函数在这里分别表示什么；
- 为什么把状态放在 `private`；
- 构造函数和初始化列表在何时执行；
- `const` 成员函数承诺了什么；
- 为什么 `Motor` 可以作为统一接口；
- `virtual`、纯虚函数与 `override` 分别在这里起什么作用；
- 为什么同一个 `Motor&` 能调用不同电机实现；
- `Robot has Motors` 与 `DMMotor is a Motor` 的区别；
- 引用成员对对象生命周期有什么要求；
- 头文件和 `.cpp` 分别承担什么角色；
- 编译错误和链接错误的区别；
- `-Iinclude` 的作用；
- CMake target 是什么；
- `add_library`、`add_executable`、`target_link_libraries` 分别描述什么。

更重要的是，应当能够做一个很小的现场修改，例如：

- 增加 `disable()` 接口，并让两种电机实现；
- 把目标位置从 `1.50` 改为 `-0.80`；
- 新增一种电机类，使 `Robot` 不需要修改；
- 故意漏掉一个源文件，判断为什么出现链接错误。

这和培训仓库的总体验收原则一致：**做出来、能运行、讲得清楚。**

---

## 11. 继续往真实机器人代码走时

本文故意没有继续引入：

- 智能指针与动态所有权；
- STL 容器；
- 模板；
- 异常；
- 线程；
- 真正电机 SDK；
- ROS 2 生命周期与节点结构。

原因不是这些内容不重要，而是它们不应该在第一次理解“类、接口、多态、组合和构建系统”时全部同时出现。

当后续代码真的遇到“电机数量不固定”“对象需要动态创建”“多个模块共享资源”“设备通信失败”等问题时，再引入对应工具，会比现在一次性把所有 C++ 机制塞进示例更容易理解。
