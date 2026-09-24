# CMake

上一节已经可以手动编译多文件工程：

```bash
g++ apps/main.cpp \
    src/dm_motor.cpp \
    src/unitree_motor.cpp \
    src/robot.cpp \
    -Iinclude -std=c++17 -o robot_demo
```

文件继续增加以后，手工维护这条命令会越来越麻烦。CMake 用来记录工程中的构建关系，再生成 Makefile、Ninja 等构建系统需要的文件。

一个简化后的流程是：

```text
CMakeLists.txt
      ↓
    CMake
      ↓
Makefile / Ninja
      ↓
  构建工具
      ↓
     g++
      ↓
库和可执行文件
```

因此 CMake 本身不负责把 C++ 源码编译成机器代码，真正执行编译的仍然是编译器。

## executable target

最简单的 `CMakeLists.txt` 可以直接建立一个可执行目标：

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

这里定义了一个名为 `robot_demo` 的 target。

`add_executable()` 指定它由哪些源文件组成：

```cmake
add_executable(robot_demo
    ...
)
```

`target_include_directories()` 给这个 target 增加头文件搜索路径：

```cmake
target_include_directories(robot_demo PRIVATE include)
```

这样编译时就能找到：

```cpp
#include "robot/motor.hpp"
```

## library target

核心机器人代码还可以单独编译成库：

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

这里有两个 target：

- `robot_core`：包含电机和机器人实现；
- `robot_demo`：包含 `main()`，生成最终可执行程序。

`target_link_libraries()` 表示 `robot_demo` 需要链接 `robot_core`。

### PUBLIC 与 PRIVATE

`robot_core` 的公开头文件位于 `include/`。使用 `robot_core` 的其他 target 也需要这个 include 路径，因此写：

```cmake
target_include_directories(robot_core PUBLIC include)
```

`robot_demo` 对 `robot_core` 的依赖只属于自己：

```cmake
target_link_libraries(robot_demo PRIVATE robot_core)
```

后面接触更复杂的 CMake 项目时还会遇到 `INTERFACE`。当前练习先掌握 `PUBLIC` 和 `PRIVATE` 的基本含义。

## 单独使用 build 目录

推荐把构建产物放在源码目录之外：

```bash
cmake -S . -B build
cmake --build build
```

第一条命令读取当前目录中的 `CMakeLists.txt`，并把生成的构建文件写入 `build/`。

第二条命令执行实际构建。

完成后运行：

```bash
./build/robot_demo
```

这样中间文件、目标文件和可执行程序都集中在 `build/` 中。需要重新配置时，也可以直接删除 `build/` 后重新生成。

## 动手：完成 mini_robot 多文件工程

工程骨架：

- [starter/05_cmake](./starter/05_cmake/)

目录结构：

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

需要完成：

1. 三个 `src/*.cpp` 中的 TODO；
2. `CMakeLists.txt`；
3. 使用 CMake 构建 `robot_demo`；
4. 运行并核对输出；
5. 能说明头文件搜索路径、library target、executable target 和链接关系。

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

完成以后，把：

```cpp
robot.move(1.50);
```

改成：

```cpp
robot.move(-0.80);
```

重新构建并运行，检查相关输出是否都变成 `-0.80 rad`。

还可以暂时从 `CMakeLists.txt` 中去掉一个 `.cpp`，重新构建并观察链接器给出的错误。确认错误以后再把源文件加回来。
