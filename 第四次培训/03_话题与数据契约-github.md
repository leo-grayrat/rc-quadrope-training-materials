# 用话题连接状态和控制量

上一节已经能运行一个节点。这一篇只增加一件能力：**让控制器通过 ROS2 收到关节状态，并把算出的关节力矩发出去。**

先用一个关节做通，再扩到 12 个关节。这样第一次学习发布/订阅时，不需要同时处理完整机器狗、MuJoCo 循环和消息映射。

## 1. 话题是有名字、有类型的数据流

控制器需要两条数据流：

```text
/joint_states
执行端 ─────────────→ 控制器

/joint_torques
执行端 ←───────────── 控制器
```

发送消息的一侧创建 **发布者**，接收消息的一侧创建 **订阅者**。

一条话题能否正常连接，至少取决于：

- 话题名是否一致；
- 消息类型是否一致；
- 双方是否对字段的含义有同一份约定。

前两项一致只能保证 ROS2 知道怎样传输数据，不能保证机器人语义正确。两个程序都发送 12 个 `double`，如果一个按 `FL_hip, FL_thigh, ...` 排列，另一个按另一种顺序解释，类型仍然完全匹配，控制结果却会错。

## 2. 先固定这次练习的消息契约

状态使用 ROS2 已有的：

```text
sensor_msgs/msg/JointState
```

这次只使用其中三个字段：

```text
name[]      关节名
position[]  位置，rad
velocity[]  速度，rad/s
```

控制器输出力矩暂时使用：

```text
std_msgs/msg/Float64MultiArray
```

其中：

```text
data[]  关节侧力矩，N·m
```

单关节练习里 `data[0]` 就是唯一关节的力矩。进入 12 关节任务后，`data[]` 的顺序固定为第三次程序里的 `joint_order`。

## 3. 一个关节的控制器节点

把上一节的 `hello_node.cpp` 暂时放到一边，新建：

```text
robot_controller/src/controller_node.cpp
```

代码如下：

```cpp
#include <memory>

#include "rclcpp/rclcpp.hpp"
#include "sensor_msgs/msg/joint_state.hpp"
#include "std_msgs/msg/float64_multi_array.hpp"

class ControllerNode : public rclcpp::Node
{
public:
  ControllerNode() : Node("controller_node")
  {
    torque_pub_ = this->create_publisher<std_msgs::msg::Float64MultiArray>(
      "/joint_torques", 10);

    state_sub_ = this->create_subscription<sensor_msgs::msg::JointState>(
      "/joint_states", 10,
      [this](const sensor_msgs::msg::JointState::SharedPtr msg) {
        state_callback(msg);
      });
  }

private:
  void state_callback(const sensor_msgs::msg::JointState::SharedPtr msg)
  {
    if (msg->position.empty() || msg->velocity.empty()) {
      return;
    }

    const double q = msg->position[0];
    const double dq = msg->velocity[0];

    const double q_des = 0.0;
    const double dq_des = 0.0;
    const double kp = 20.0;
    const double kd = 1.0;

    const double tau = kp * (q_des - q) + kd * (dq_des - dq);

    std_msgs::msg::Float64MultiArray out;
    out.data = {tau};
    torque_pub_->publish(out);
  }

  rclcpp::Publisher<std_msgs::msg::Float64MultiArray>::SharedPtr torque_pub_;
  rclcpp::Subscription<sensor_msgs::msg::JointState>::SharedPtr state_sub_;
};

int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<ControllerNode>());
  rclcpp::shutdown();
  return 0;
}
```

现在的数据流已经能从代码里直接读出来：

```text
/joint_states
      ↓ subscription callback
q / dq
      ↓ PD
 tau
      ↓ publisher
/joint_torques
```

第一次接触订阅回调时，不需要再引入线程。`rclcpp::spin(...)` 会等待和处理消息；收到 `/joint_states` 后，回调被调用，回调里完成一次控制计算并发布结果。

## 4. 把新节点加入构建

在 `CMakeLists.txt` 里增加：

```cmake
add_executable(controller_node src/controller_node.cpp)
ament_target_dependencies(
  controller_node
  rclcpp
  sensor_msgs
  std_msgs)
```

并把它加入原有 `install(TARGETS ...)`：

```cmake
install(TARGETS
  hello_node
  controller_node
  DESTINATION lib/${PROJECT_NAME})
```

重新编译并 source：

```bash
cd ~/ros2_ws
colcon build --symlink-install --packages-select robot_controller
source install/setup.bash
```

启动：

```bash
ros2 run robot_controller controller_node
```

## 5. 不写第二个节点，先用命令行当“假执行端”

另开终端，先观察控制器输出：

```bash
ros2 topic echo /joint_torques
```

再开一个终端，手工发一次状态：

```bash
ros2 topic pub --once \
  /joint_states \
  sensor_msgs/msg/JointState \
  "{name: ['joint1'], position: [0.5], velocity: [0.1]}"
```

按代码里的参数：

```math
\tau
=20(0-0.5)+1(0-0.1)
=-10.1\ \mathrm{N\cdot m}.
```

所以 `/joint_torques` 应该出现接近：

```text
data:
- -10.1
```

这一小步非常重要：**发布/订阅是否接通，可以在 MuJoCo 还没接进来之前独立验证。**

## 6. 用 ROS2 图检查连接关系

节点运行时：

```bash
ros2 node list
ros2 node info /controller_node
ros2 topic list -t
ros2 topic info /joint_states
ros2 topic info /joint_torques
```

你应该能确认：

```text
/controller_node
  subscribes: /joint_states [sensor_msgs/msg/JointState]
  publishes:  /joint_torques [std_msgs/msg/Float64MultiArray]
```

当后面的 MuJoCo 节点接上以后，`topic info` 还能告诉你相应话题上是否真的出现了另一端的发布者或订阅者。

## 7. `10` 到底是什么

代码里的：

```cpp
create_publisher<...>("/joint_torques", 10)
create_subscription<...>("/joint_states", 10, callback)
```

这里的 `10` 是用简写方式创建 QoS 时的历史深度：默认采用 keep-last，只保留最近若干条样本。

它不应该被记成“最多缓存 10 条，所以 ROS2 第 11 条一定挤掉第 1 条”这样一条完整通信定律。实际传输还受可靠性、DDS 实现、发送和处理速度等因素影响。第四次只需要知道：**双方除了消息类型，还存在 QoS 这一层通信策略；当前先使用默认策略，等真正遇到高频传感器或跨机器问题时再单独学习。**

## 8. 练习：把一个关节改成 12 个关节

**产物**：`controller_node` 能从一条 `JointState` 中取得第三次程序需要的 12 个 `q/dq`，并发布 12 个关节力矩。

**边界**：

- `JointState.name`、`position`、`velocity` 的对应关系已经给定；
- 输出力矩顺序使用第三次程序已有的 `joint_order`；
- PD、阻尼和轨迹算法直接复用第三次已经完成的逻辑；
- 本题不要求自定义 ROS2 消息，也不要求多线程。

实现时不要假定 `JointState` 中的数组顺序永远恰好等于 `joint_order`。先根据 `name` 建立名字到数组下标的对应，再按自己的 `joint_order` 取值。

**判定方式**：先不用 MuJoCo，通过 `ros2 topic pub` 手工发一条 12 关节状态；再用：

```bash
ros2 topic echo /joint_torques
```

确认：

- 输出恰好有 12 个元素；
- 顺序与 `joint_order` 一致；
- 用一个自己能手算的输入检查至少一个关节的力矩值。

这一关通过后，再把真正执行端接进来。
