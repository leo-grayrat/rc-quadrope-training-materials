# 工作空间、功能包与第一个 C++ 节点

这一篇只解决一个问题：**一份 C++ 程序怎样进入 ROS2 的构建和运行体系，并真正变成一个可以被 ROS2 发现的节点。**

第三次已经学过 C++、多文件工程和 CMake，所以这里不重新讲 CMake 语法，只解释 ROS2 在原有构建流程上增加了什么。

## 1. 四个层次先分清

ROS2 初学时最容易把“目录里的东西”和“运行起来的东西”混成一棵树。更准确的关系是：

| 层次 | 是什么 | 这次的例子 |
|---|---|---|
| 工作空间 workspace | 一组 ROS2 包共同开发、编译的目录 | `~/ros2_ws` |
| 功能包 package | 源码、依赖、构建规则、launch 等的组织单位 | `robot_controller` |
| 可执行程序 executable | 编译后可以被 `ros2 run` 启动的程序 | `hello_node` |
| 节点 node | 程序运行后创建的 ROS2 运行时对象 | `/hello_node` |

一个包可以生成多个可执行程序；一个程序也可以创建多个节点。本次为了让结构简单，先采用“一份可执行程序创建一个主要节点”。

## 2. 建立工作空间和 C++ 包

先让当前终端认识 ROS2 Humble：

```bash
source /opt/ros/humble/setup.bash
```

建立工作空间：

```bash
mkdir -p ~/ros2_ws/src
cd ~/ros2_ws/src
```

创建一个 C++ 包：

```bash
ros2 pkg create \
  --build-type ament_cmake \
  robot_controller \
  --dependencies rclcpp std_msgs sensor_msgs
```

这里真正新增的信息只有三件：

- `ament_cmake`：这个包使用 ROS2 的 CMake 构建方式；
- `rclcpp`：C++ 的 ROS2 客户端库；
- `std_msgs`、`sensor_msgs`：后面话题练习会使用的标准消息包。

创建后主要会看到：

```text
robot_controller/
├── CMakeLists.txt
├── package.xml
├── include/robot_controller/
└── src/
```

`package.xml` 记录包的元信息和依赖；`CMakeLists.txt` 负责把源码编译成可执行程序并安装到 ROS2 能找到的位置。

## 3. 写一个最小节点

在 `robot_controller/src/hello_node.cpp` 中写：

```cpp
#include <chrono>
#include <memory>

#include "rclcpp/rclcpp.hpp"

using namespace std::chrono_literals;

class HelloNode : public rclcpp::Node
{
public:
  HelloNode() : Node("hello_node")
  {
    timer_ = this->create_wall_timer(1s, [this]() {
      RCLCPP_INFO(this->get_logger(), "hello ros2");
    });
  }

private:
  rclcpp::TimerBase::SharedPtr timer_;
};

int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<HelloNode>());
  rclcpp::shutdown();
  return 0;
}
```

这段代码里现在只需要建立四条关系：

```text
rclcpp::init
    ↓ 初始化 ROS2 客户端库
HelloNode
    ↓ 创建节点对象
create_wall_timer
    ↓ 定时产生事件
rclcpp::spin
    ↓ 持续处理定时器、订阅回调等事件
```

`spin` 不是“让程序凭空一直运行”的咒语。节点里的定时器、订阅者等对象产生事件后，需要执行器去调度对应回调；这里 `rclcpp::spin(...)` 使用默认执行器持续处理这些工作。

## 4. 把这个程序交给 CMake

在 `CMakeLists.txt` 的 `ament_package()` 之前加入：

```cmake
add_executable(hello_node src/hello_node.cpp)
ament_target_dependencies(hello_node rclcpp)

install(TARGETS
  hello_node
  DESTINATION lib/${PROJECT_NAME})
```

三行关系和普通 CMake 很接近：

```text
src/hello_node.cpp
      ↓ add_executable
hello_node 可执行程序
      ↓ dependencies
链接 rclcpp
      ↓ install
放进 ROS2 约定的安装位置
```

如果只写了 `add_executable()` 却没有 `install()`，源码可能编译成功，但 `ros2 run robot_controller hello_node` 仍然找不到它。

## 5. 编译、source、运行

回到工作空间根目录：

```bash
cd ~/ros2_ws
colcon build --symlink-install --packages-select robot_controller
```

成功后会出现：

```text
ros2_ws/
├── src/
├── build/
├── install/
└── log/
```

这里仍然只有 `src/` 是主要源码目录。`build/`、`install/`、`log/` 都是构建产物，不应该手工复制源码进去。

让当前终端认识刚编译出的包：

```bash
source ~/ros2_ws/install/setup.bash
```

然后运行：

```bash
ros2 run robot_controller hello_node
```

终端应该每秒出现一次：

```text
[INFO] [...] [hello_node]: hello ros2
```

## 6. 不看源码，验证 ROS2 是否真的认识它

另开一个终端，同样先 source：

```bash
source /opt/ros/humble/setup.bash
source ~/ros2_ws/install/setup.bash
```

查看节点：

```bash
ros2 node list
```

应该能看到：

```text
/hello_node
```

再看这个节点对外暴露了什么：

```bash
ros2 node info /hello_node
```

现在它还没有我们自己创建的话题，只会显示 ROS2 节点自带的一些接口。下一篇加入发布者和订阅者后，这条命令会变得更有用。

## 7. 这次练习的验收

**产物**：一个能够被 `ros2 run` 启动的 `hello_node`。

**给定边界**：包名、可执行程序名、节点名都按本篇使用；本题不要求自己设计程序结构。

**判定方式**：

```text
colcon build 成功
        ↓
ros2 run 能启动
        ↓
ros2 node list 出现 /hello_node
        ↓
终端持续打印 hello ros2
```

如果编译成功但 `ros2 run` 找不到程序，优先检查 `install(TARGETS ...)` 和当前终端是否重新 `source install/setup.bash`，不要先怀疑 ROS2 “没有识别源码”。
