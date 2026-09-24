# 05_cmake

本目录对应 [CMake](../../06_CMake.md) 中的多文件工程练习。

需要完成：

1. `src/dm_motor.cpp`
2. `src/unitree_motor.cpp`
3. `src/robot.cpp`
4. `CMakeLists.txt`

`apps/main.cpp` 和公开头文件已经给定。

## 构建

```bash
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

需要单独检查 C++ 实现时，也可以手动编译：

```bash
g++ apps/main.cpp \
    src/dm_motor.cpp \
    src/unitree_motor.cpp \
    src/robot.cpp \
    -Iinclude -std=c++17 -o robot_demo
./robot_demo
```

## 练习后的修改

把 `apps/main.cpp` 中：

```cpp
robot.move(1.50);
```

改为：

```cpp
robot.move(-0.80);
```

重新构建，检查输出中的目标位置和两台电机位置。

还可以暂时从 CMake target 中去掉一个 `.cpp`，重新构建并查看链接错误。观察完成后恢复原来的源文件列表。
