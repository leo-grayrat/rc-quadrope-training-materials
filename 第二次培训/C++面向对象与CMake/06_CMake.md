# CMake

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

## 最简单的可执行 target

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

## 为什么还会有 library target

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

## out-of-source build

推荐：

```bash
cmake -S . -B build
cmake --build build
./build/robot_demo
```

构建产物集中在 `build/`，源码目录保持干净。需要完全重新配置时，也可以删除 `build/` 后重建，而不必手工寻找散落在源码目录里的中间文件。

---

## 动手：完成 mini_robot 多文件工程

仓库中提供：

- [starter/05_cmake](./starter/05_cmake/)

这一次不再要求从空目录猜最终工程应该长什么样。目录、头文件接口和测试入口已经提供，你需要补齐实现与构建描述。

目标结构：

```text
05_cmake/
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
cd '第二次培训/C++面向对象与CMake/starter/05_cmake'
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
