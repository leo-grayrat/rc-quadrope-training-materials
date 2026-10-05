<!-- 从 2026-10-04 发布的 PDF 反向恢复 Markdown 结构；正文内容保持原意，仅校正 PDF 提取造成的断行与代码缩进。 -->

# ros2 基本概念

本篇面向已经写过一个 MuJoCo 仿真程序的队员：先讲清楚“为什么需要 ros2”，再动手用 Python 和 C++ 各建立一个最小节点，然后把 ros2 的三种通信方式和 launch 文件过一遍。看完之后，你应该能自己建工作空间、写节点、用话题收发消息，并能看懂别人的 ros2 程序。

## 一、ros2 介绍，以及工作空间结构

### 1.1 从一个仿真程序说起：控制器层与仿真运行层

我们之前做了仿真程序。从逻辑上抽象分层，可以分成两部分：**控制器层和仿真运行层**。

- **控制器层**：负责让机器人阻尼、站立、行走这类命令的计算。它需要知道机器人当前的状态，然后算出应该给机器人什么命令。
- **仿真运行层**：接收控制器计算好的命令并执行，并且向控制器反馈当前仿真中机器人的状态。

两者的关系可以用下面这张简图表示：

> 图 1：控制器层与仿真运行层之间的信息交换

从图里可以看到，两层之间交换的信息其实只有两类：控制器层发下去的**控制命令**，和仿真运行层反馈回来的**机器人状态**。

### 1.2 把仿真换成实机：接口上的启示

接下来我们从接口的角度看这件事。仿真运行层对外做的事情是“接收命令、反馈状态”，而实机做的事情同样是“接收命令、反馈自身状态”。也就是说，从接口来看，仿真运行层做的就是代替实机。

因此，我们可以直接把仿真运行层替换成实体机器人的运行程序，控制程序可以不用变；只需要把仿真运行程序切换成实机运行程序，就能用相同的控制器控制实机。

> 图 2：控制器不变，只把仿真运行层替换成实机运行程序

这在逻辑上是没有任何疑问的。

### 1.3 问题：分层在代码里体现得不直接

但问题是，大家的程序中，这样的分层体现得不是很直接：仿真程序和控制器程序混合在一个 `.py` 文件里面。仿真循环、`mj_step`、控制器计算、状态读取全都缠在一起。

我们怎么才能在代码上把上述的分层、切换这种操作给体现出来呢？于是我们引入 ros2。

### 1.4 ros2 的作用：把两层拆成可以独立运行的程序

有了 ros2 之后，我们可以直接把仿真运行程序和控制器程序写成两个能独立运行的程序，两个独立运行的程序之间使用 ros2 通信——通信就是交换信息。这种独立运行的程序，我们一般叫做节点（node）。

为了达到上述的效果，我们需要三个节点：仿真节点、控制器节点、实机节点。启动控制器节点让它独立运行，这时如果启动仿真节点，控制器就会控制仿真；如果启动实机节点，控制器就会控制实机。

> 图 3：一个控制器节点，同一时刻只连接仿真节点或实机节点中的一个

这样做了之后，你的仿真程序和控制器程序不再耦合在一块了，因此我们把这叫做解耦。

解耦好处的本质是让程序得以复用。比如上面的例子里，我们就让控制器程序得以复用；此外，我们不难联想到，这种得以复用的好处对于代码开源来说也是很好的事情：其他人的程序只要是一个 ros2 节点，我们把它独立运行起来，然后用自己的程序去和开源程序做信息的交换就可以了。

比如说雷达的定位程序：网上开源的 ros2 程序我们可以拿来直接运行，在我们自己的程序里面只要写好接收语句，就能直接利用网上开源的雷达定位程序。

了解了 ros2 的好处和存在的必要，我们就正式开始介绍 ros2。由简入繁，我们先来动手建立一个最小的 ros2 节点，并借此了解 ros2 的骨架结构。

> 一个提醒：仿真节点和实机节点不要同时启动。它们会订阅同样的指令话题、发布同样的状态话题，控制器就分不清自己在控制谁了。确实需要同时运行时，可以用话题重映射（remap）把名字改开，这是后话，这里先不展开。

### 1.5 动手：建立第一个 ros2 节点，打印 hello ros2

这一节的目标：从零建好工作空间和功能包，分别用 Python 和 C++ 写一个节点，让它每秒在终端打印一次 `hello ros2`。

#### 1.5.1 先分清三个概念：工作空间、包、节点

在动手之前，先把三个最容易混淆的概念分清楚，后面的命令都是围绕它们展开的：

| 概念 | 是什么 | 例子 |
|---|---|---|
| 工作空间 workspace | 一个用来放代码和编译产物的总目录，里面可以装任意多个包 | `~/ros2_ws`，里面有 `src/build/install/log` |
| 包 package（功能包） | ros2 里代码组织和编译的最小单位，用包名标识；包里可以放 Python/C++ 代码、launch、配置等 | `py_hello`、`cpp_hello` |
| 节点 node | 包里的一个程序运行起来之后的实例；ros2 通信的主体是节点，运行时用节点名标识 | `ros2 run cpp_hello talker` 启动出来的就是 talker 节点 |

三者的关系是：工作空间 ⊃ 包 ⊃ 节点。展开来说：

- 一个工作空间可以有多个包，比如我们马上就要建一个 Python 包 `py_hello` 和一个 C++ 包 `cpp_hello`；
- 一个包可以定义多个节点，比如 `cpp_hello` 里会有 `hello_node`、`talker`、`listener` 三个可执行程序，每个运行起来都是一个节点；
- 包和源码是“文件”层面的概念，节点是“运行进程”层面的概念：源码写在包里，用 `ros2 run`（或 launch）把包里的某个程序跑起来，才产生节点。

#### 1.5.2 创建工作空间和 Python 包

ros2 的代码统一放在一个**工作空间（workspace）**里管理。工作空间本质上就是一个目录，标准结构如下：

```text
ros2_ws/
├── src/        # 源码，我们写的功能包都放在这里
├── build/      # 编译中间文件，colcon 自动生成
├── install/    # 编译产物，运行前 source 的就是这里
└── log/        # 编译日志，colcon 自动生成
```

只有 `src` 是我们手动管理的，其余三个目录都是编译时自动生成的。先创建工作空间和第一个功能包：

```bash
mkdir -p ~/ros2_ws/src
cd ~/ros2_ws/src
ros2 pkg create --build-type ament_python py_hello --dependencies rclpy
```

解释一下这条命令：

- `ros2 pkg create`：创建一个新的功能包；
- `--build-type ament_python`：这是一个 Python 包（先建它做演示，C++ 包 1.5.6 再建）；
- `py_hello`：包名，后面 `ros2 run` 的时候要用到；
- `--dependencies rclpy`：声明依赖 `rclpy`，它是 ros2 的 Python 客户端库，我们写 Python 节点全靠它。

创建完成后，`src` 下面会多出一个 `py_hello` 目录，结构如下：

```text
py_hello/
├── package.xml
├── setup.py
├── setup.cfg
├── resource/py_hello
├── py_hello/
│   └── __init__.py
└── test/
```

#### 1.5.3 编写 Python 节点代码

在 `py_hello/py_hello/` 目录下新建一个文件 `hello_node.py`：

```python
import rclpy
from rclpy.executors import ExternalShutdownException
from rclpy.node import Node


class HelloNode(Node):
    def __init__(self):
        super().__init__('hello_node')
        self.timer = self.create_timer(1.0, self.timer_callback)

    def timer_callback(self):
        self.get_logger().info('hello ros2')


def main(args=None):
    rclpy.init(args=args)
    node = HelloNode()
    try:
        rclpy.spin(node)
    except (KeyboardInterrupt, ExternalShutdownException):
        pass
    finally:
        node.destroy_node()
        if rclpy.ok():
            rclpy.shutdown()


if __name__ == '__main__':
    main()
```

逐段看这段代码：

1. `class HelloNode(Node)`：定义一个节点类，继承 ros2 提供的 `Node` 基类。`super().__init__('hello_node')` 里的字符串是节点名。
2. `self.create_timer(1.0, self.timer_callback)`：创建一个定时器，每 1 秒调用一次 `timer_callback`。节点的能力都是这样“挂”在节点对象上的：定时器、发布者、订阅者都一样。
3. `self.get_logger().info(...)`：打印日志。它比 `print` 多了时间戳和节点名，还能在 launch 里统一管理，是 ros2 程序里推荐的打印方式。
4. `rclpy.init / rclpy.spin / rclpy.shutdown`：这三步是所有 Python 节点的标准生命周期。spin 直译是“旋转”，作用是让程序停在这里循环，一旦有事件（定时器到点、收到消息……）就去调用对应的回调函数；没有这一句，程序建立完节点就直接退出了。

#### 1.5.4 注册 Python 节点：修改 setup.py

打开包根目录下的 `setup.py`，找到 `entry_points` 里的 `console_scripts`，注册节点：

```python
entry_points={
    'console_scripts': [
        'hello_node = py_hello.hello_node:main',
    ],
},
```

#### 1.5.5 编译与运行（Python 版）

```bash
cd ~/ros2_ws
colcon build --symlink-install
source install/setup.bash
ros2 run py_hello hello_node
```

应该能看到每秒打印一行：

```text
[INFO] [1791129451.435303174] [hello_node]: hello ros2
[INFO] [1791129452.427324844] [hello_node]: hello ros2
...
```

> 重要习惯：每开一个新终端，都要先 source 环境：
>
> ```bash
> source /opt/ros/humble/setup.bash
> source ~/ros2_ws/install/setup.bash
> ```

#### 1.5.6 C++ 版：用同样的流程再走一遍

```bash
cd ~/ros2_ws/src
ros2 pkg create --build-type ament_cmake cpp_hello --dependencies rclcpp std_msgs
```

生成的目录结构：

```text
cpp_hello/
├── CMakeLists.txt
├── package.xml
├── include/cpp_hello/
└── src/
```

在 `cpp_hello/src/` 下新建 `hello_node.cpp`：

```cpp
#include <chrono>
#include <functional>
#include <memory>

#include "rclcpp/rclcpp.hpp"

using namespace std::chrono_literals;

class HelloNode : public rclcpp::Node
{
public:
  HelloNode() : Node("hello_node")
  {
    timer_ = this->create_wall_timer(
      1s, std::bind(&HelloNode::timer_callback, this));
  }

private:
  void timer_callback()
  {
    RCLCPP_INFO(this->get_logger(), "hello ros2");
  }

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

在 `CMakeLists.txt` 中加入：

```cmake
add_executable(hello_node src/hello_node.cpp)
ament_target_dependencies(hello_node rclcpp)

install(TARGETS
  hello_node
  DESTINATION lib/${PROJECT_NAME})
```

编译运行：

```bash
cd ~/ros2_ws
colcon build --symlink-install
source install/setup.bash
ros2 run cpp_hello hello_node
```

#### 1.5.7 节点的骨架小结

> 图 4：Python 节点与 C++ 节点的骨架对照

后面写的所有节点，不管多复杂，都是在这五步的框架里往第 3 步加东西。

#### 1.5.8 为什么控制器节点要用 C++ 写

两个 `hello ros2` 节点功能完全一样，那后面的程序到底用哪种语言写？我们的答案是：**控制器节点用 C++ 写。**

原因很直接：针对实时机器人控制的场景，运行效率很重要，C++ 的运行效率相比 Python 更高。具体来说：

- 控制回路通常要跑到几百 Hz 甚至 1 kHz，每个周期里要做矩阵运算、状态估计、关节命令计算，还要保证周期尽量稳定；
- Python 是解释执行，还有 GIL（全局解释器锁）和垃圾回收带来的抖动，控制周期容易忽长忽短；
- C++ 是编译型语言，没有解释器和 GIL 的开销，内存和线程也更容易精确控制，更适合对实时性有要求的场合。

所以从第二节开始，我们的例子程序全部用 C++ 写，正式的控制器节点也用 C++ 实现。Python 也不是没有用武之地：写脚本、做数据分析、快速验证算法都很方便，只是关键的控制回路交给 C++ 更稳妥。

### 1.6 常用 ros2 命令速查

| 命令 | 作用 |
|---|---|
| `ros2 run <包名> <可执行文件名>` | 运行一个节点 |
| `ros2 node list` | 列出当前正在运行的节点 |
| `ros2 node info /节点名` | 查看某节点的发布者、订阅者、服务 |
| `ros2 topic list` | 列出当前所有话题 |
| `ros2 topic echo /话题名` | 把话题上流过的消息打印到终端 |
| `ros2 topic info /话题名` | 查看话题的消息类型、发布者/订阅者数量 |
| `ros2 topic hz /话题名` | 统计话题的发布频率 |
| `ros2 topic pub --once /hello std_msgs/msg/String "{data: 'hello'}"` | 手动发一条消息 |
| `ros2 interface show <消息类型>` | 查看消息类型内部字段 |
| `ros2 pkg list / ros2 pkg executables <包名>` | 查看系统里的包 / 某个包里的可执行程序 |
| `colcon build --packages-select <包名>` | 只编译某个包 |
| `ros2 launch <包名> <launch文件名>` | 用 launch 文件启动 |

## 二、ros2 的通信方式：话题，服务，参数

节点之间交换信息的方式主要有三种：话题、服务、参数（严格说参数不是“通信”，而是节点对外的配置接口，习惯上放在一起讲）。

### 2.1 话题通信：单向的信息传递

#### 2.1.1 概念

控制器节点创建一个发布者（publisher），向 `/joint_commands` 发布消息；仿真节点创建订阅者（subscriber）接收。反过来，仿真节点发布 `/joint_states`，控制器节点订阅 `/joint_states`。

> 图 5：控制器节点与仿真节点之间的两条话题

这里有两个要点：

1. 话题的名字必须完全一致（区分大小写和斜杠），消息类型也必须一致；
2. 发布者只管发，不关心有没有人在听；订阅者只管收，不关心是谁发的。

#### 2.1.2 例程：talker 与 listener（C++ 版）

`talker.cpp`：

```cpp
#include <algorithm>
#include <chrono>
#include <functional>
#include <memory>
#include <string>

#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/string.hpp"

class Talker : public rclcpp::Node
{
public:
  Talker() : Node("talker")
  {
    this->declare_parameter<std::string>("message", "hello");
    this->declare_parameter<int>("period_ms", 1000);
    message_ = this->get_parameter("message").as_string();
    const int period_ms =
      std::max(static_cast<int>(this->get_parameter("period_ms").as_int()), 1);

    publisher_ = this->create_publisher<std_msgs::msg::String>("hello", 10);
    timer_ = this->create_wall_timer(
      std::chrono::milliseconds(period_ms),
      std::bind(&Talker::timer_callback, this));
  }

private:
  void timer_callback()
  {
    auto msg = std_msgs::msg::String();
    msg.data = message_;
    publisher_->publish(msg);
    ++count_;
    RCLCPP_INFO(this->get_logger(), "发布第 %zu 条: %s", count_, msg.data.c_str());
  }

  rclcpp::Publisher<std_msgs::msg::String>::SharedPtr publisher_;
  rclcpp::TimerBase::SharedPtr timer_;
  std::string message_;
  size_t count_ = 0;
};

int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<Talker>());
  rclcpp::shutdown();
  return 0;
}
```

`listener.cpp`：

```cpp
#include <functional>
#include <memory>

#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/string.hpp"

class Listener : public rclcpp::Node
{
public:
  Listener() : Node("listener")
  {
    subscription_ = this->create_subscription<std_msgs::msg::String>(
      "hello", 10,
      std::bind(&Listener::callback, this, std::placeholders::_1));
  }

private:
  void callback(const std_msgs::msg::String::SharedPtr msg) const
  {
    RCLCPP_INFO(this->get_logger(), "收到: %s", msg->data.c_str());
  }

  rclcpp::Subscription<std_msgs::msg::String>::SharedPtr subscription_;
};

int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<Listener>());
  rclcpp::shutdown();
  return 0;
}
```

CMake 注册：

```cmake
add_executable(talker src/talker.cpp)
ament_target_dependencies(talker rclcpp std_msgs)

add_executable(listener src/listener.cpp)
ament_target_dependencies(listener rclcpp std_msgs)

install(TARGETS
  hello_node
  talker
  listener
  DESTINATION lib/${PROJECT_NAME})
```

运行：

```bash
ros2 run cpp_hello talker
ros2 run cpp_hello listener
```

观察：

```bash
ros2 topic list -t
ros2 topic info /hello
ros2 topic echo /hello
ros2 topic hz /hello
```

#### 2.1.3 话题通信的两个角色

| 角色 | 创建方式 | 关键动作 |
|---|---|---|
| 发布者 Publisher | `create_publisher<消息类型>("话题名", 队列长度)` | 调用 `publish(msg)` |
| 订阅者 Subscriber | `create_subscription<消息类型>("话题名", 队列长度, 回调)` | 收到消息后自动调用回调函数 |

队列长度写 10，表示“最多缓存 10 条消息”：如果发布太快、订阅者处理不过来，新的消息会挤掉旧的消息。现在统一写 10 就行，更复杂的场景会用到 QoS 配置，需要时再查。

### 2.2 服务通信：双向的请求与回应

#### 2.2.1 概念

在服务通信中，一方是客户端（Client），另一方是服务端（Server）。客户端提出要求，服务端完成要求后将结果发回客户端。

> 图 6：服务通信中的请求与响应

一个服务由服务名称、请求（Request）的数据结构、响应（Response）的数据结构定义。

#### 2.2.2 例子：用服务做“握手”

遥控器节点想让导航节点开始导航，如果用话题传递这个消息，消息丢失会很尴尬。使用服务可以要求导航节点回一个“收到了”。

ros2 的通信底层实际上是网络通信，在同一个局域网内，不同机器上的 ros2 节点也可以相互发消息。

> 跨机器的前提：两台机器在同一个局域网、能互相 ping 通，并且 `ROS_DOMAIN_ID` 一致（默认都是 0）。

#### 2.2.3 话题和服务怎么选

| | 话题 Topic | 服务 Service |
|---|---|---|
| 方向 | 单向：发布者 → 订阅者 | 双向：客户端 ⇄ 服务端 |
| 有没有回应 | 没有 | 有，服务端必须回一个响应 |
| 典型用途 | 持续不断的数据流：关节指令、机器人状态、雷达数据 | 一次性的请求：开始导航、切换模式、保存地图 |
| 丢失消息的后果 | 丢一帧还有下一帧，通常能接受 | 请求丢了就没有响应，客户端能发现 |

一句话记法：话题像广播，服务像打电话。

#### 2.2.4 服务：会“描述需求”就够了

培训阶段不需要你手写服务的代码，但你要能把需求给 AI 描述清楚。描述一个服务，至少交代这四件事：

1. 谁是客户端、谁是服务端；
2. 服务名是什么；
3. 请求里要带什么数据、响应里要带什么数据；
4. 客户端什么时候调用，拿到响应之后怎么处理。

例如可以这样向 AI 提要求：

> 帮我用 C++（ros2 Humble、rclcpp）写两个节点。服务端是导航节点，提供一个名为 `/start_navigation` 的服务：请求里包含目标点 x、y，响应里包含 success 和 message；收到请求后打印目标点，并返回 success=true。客户端是遥控器节点，启动后每隔 3 秒调用一次这个服务，并打印响应内容。

如果只是“通知一下”、不需要额外数据，可以用 ros2 自带的 `std_srvs/srv/Trigger` 类型。

```bash
ros2 service list -t
ros2 service type /服务名
ros2 service call /服务名 <服务类型> "<请求内容>"
```

### 2.3 参数：可以在线调整的变量

参数不是节点之间的通信，而是节点对外的配置接口：每个节点可以声明若干个参数，外部可以读取和修改它们。

```cpp
this->declare_parameter<std::string>("message", "hello");
this->declare_parameter<int>("period_ms", 1000);

message_ = this->get_parameter("message").as_string();
const int period_ms = this->get_parameter("period_ms").as_int();
```

启动节点时可以指定参数：

```bash
ros2 run cpp_hello talker --ros-args -p message:=你好ros2 -p period_ms:=500
```

运行过程中可以查看和修改：

```bash
ros2 param list
ros2 param get /talker message
ros2 param set /talker message 你好
```

> 注意：我们的 talker 只在启动时读一次参数，所以 `param set` 虽然会显示成功，但不会立刻改变发布内容。想让参数在线生效，需要在代码里每次使用前重新读取，或者写参数回调。

参数适合放这些量：控制器的 `kp`、`kd`，仿真步长，最大速度，话题名字，调试开关。给 AI 提需求时，也可以直接说“把 kp、kd 做成参数，默认值是……”，它就知道该怎么写了。

## 三、launch 文件

### 3.1 要解决的问题

启动多个 ros2 节点时，要多开几个终端、多用几次 `ros2 run`。可以写一个 launch 文件，通过一个文件启动多个节点。

### 3.2 写一个 launch 文件：一键启动 talker 和 listener

在 `cpp_hello` 包根目录下新建 `launch/talk_listener.launch.py`：

```python
from launch import LaunchDescription
from launch_ros.actions import Node


def generate_launch_description():
    return LaunchDescription([
        Node(
            package='cpp_hello',
            executable='talker',
            name='talker',
            output='screen',
        ),
        Node(
            package='cpp_hello',
            executable='listener',
            name='listener',
            output='screen',
        ),
    ])
```

### 3.3 注册 launch 文件并运行

```cmake
install(DIRECTORY launch
  DESTINATION share/${PROJECT_NAME})
```

然后：

```bash
cd ~/ros2_ws
colcon build --symlink-install
source install/setup.bash
ros2 launch cpp_hello talk_listener.launch.py
```

按 `Ctrl+C` 会把这次 launch 启动的所有节点一起停掉。

### 3.4 在 launch 文件里写参数

```python
Node(
    package='cpp_hello',
    executable='talker',
    name='talker',
    output='screen',
    parameters=[{
        'message': '你好，ros2',
        'period_ms': 500,
    }],
),
```

参数多了以后，还可以把它们单独写在一个 YAML 文件里：

```python
parameters=['/path/to/params.yaml']
```

### 3.5 小结

到这里，一个最小的 ros2 工程链路就完整了：

- 用工作空间和功能包管理代码，`colcon build` 编译，用 `source` 让终端认识它；
- 用节点把控制器、仿真、实机拆成能独立运行的程序，它们通过话题交换信息；
- 需要“确认对方收到”的场合用服务；希望不改代码就能调整的量做成参数；
- 用 launch 文件一键启动所有节点，并统一传入参数；
- 控制回路用 C++ 实现，保证实时性；Python 适合写脚本、做数据分析和快速验证算法。

回到最开始的目标：控制器节点和仿真/实机节点解耦之后，控制器只需要认话题，不需要知道对面是谁。以后要上实机，就是把仿真节点换成实机节点；以后想用别人的开源节点（比如雷达定位），只要它收发约定好的话题，运行起来就能直接配合——这就是 ros2 带来的复用。
