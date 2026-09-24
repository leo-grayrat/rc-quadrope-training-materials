# mini_robot starter

这是《机器人队 C++ 面向对象与 CMake 入门（DIY 版）》最后一次动手使用的起始工程。

目录、公开接口和测试入口已经给定。请完成：

1. `src/dm_motor.cpp`
2. `src/unitree_motor.cpp`
3. `src/robot.cpp`
4. `CMakeLists.txt`

不要通过修改 `apps/main.cpp` 或删除接口来绕过任务。

## 目标行为

完成后执行：

```bash
cmake -S . -B build
cmake --build build
./build/robot_demo
```

应输出：

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

如果想先绕过 CMake 检查 C++ 实现，可以手动编译：

```bash
g++ apps/main.cpp \
    src/dm_motor.cpp \
    src/unitree_motor.cpp \
    src/robot.cpp \
    -Iinclude -std=c++17 -o robot_demo
./robot_demo
```

这样可以把“C++ 实现问题”和“CMake 配置问题”分开排查。

## 完成后的现场修改

把 `apps/main.cpp` 中：

```cpp
robot.move(1.50);
```

改为：

```cpp
robot.move(-0.80);
```

重新构建并确认所有相关输出变为 `-0.80 rad`。

随后可以故意从 CMake target 中漏掉一个 `.cpp`，观察链接阶段报错，再恢复。
