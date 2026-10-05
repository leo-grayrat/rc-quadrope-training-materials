# 参数、launch 与服务边界

前四篇已经把真正的核心链路跑通：

```text
sim_node
  ↓ /joint_states
controller_node
  ↓ /joint_torques
sim_node
```

这一篇处理两个工程摩擦：

1. 控制参数不应该每次都改源码再编译；
2. 两个节点不应该每次都手开两个终端逐个启动。

最后再把 service 放回正确的位置，避免把它和 topic、parameter 混成一张“ROS2 三种通信方式”表。

## 1. 参数解决的是配置，不是节点间数据流

以控制器的 `kp`、`kd` 为例。在 `ControllerNode` 构造函数中声明：

```cpp
this->declare_parameter<double>("kp", 20.0);
this->declare_parameter<double>("kd", 1.0);

kp_ = this->get_parameter("kp").as_double();
kd_ = this->get_parameter("kd").as_double();
```

成员变量：

```cpp
double kp_;
double kd_;
```

然后控制计算使用 `kp_`、`kd_`。

现在可以在启动时覆盖默认值：

```bash
ros2 run robot_controller controller_node \
  --ros-args \
  -p kp:=30.0 \
  -p kd:=2.0
```

查看节点参数：

```bash
ros2 param list /controller_node
ros2 param get /controller_node kp
```

本节只要求**启动时读取配置**。如果代码只在构造函数中 `get_parameter()` 一次，那么运行中执行：

```bash
ros2 param set /controller_node kp 40.0
```

即使参数服务器接受了新值，你自己的 `kp_` 变量也不会自动跟着变化。想让在线修改真正进入控制计算，需要重新读取或注册参数更新回调；等实际需要在线调参时再实现，不在这里假装“参数天然在线生效”。

## 2. launch 解决多节点启动和启动配置

在 `robot_controller` 包中新建：

```text
launch/sim_control.launch.py
```

内容：

```python
from launch import LaunchDescription
from launch_ros.actions import Node


def generate_launch_description():
    return LaunchDescription([
        Node(
            package='mujoco_bridge',
            executable='sim_node',
            name='sim_node',
            output='screen',
        ),
        Node(
            package='robot_controller',
            executable='controller_node',
            name='controller_node',
            output='screen',
            parameters=[{
                'kp': 20.0,
                'kd': 1.0,
            }],
        ),
    ])
```

这个文件做的事情非常具体：

```text
描述要启动哪些可执行程序
        ↓
给运行时节点指定名字和参数
        ↓
由 ros2 launch 一起启动和管理
```

它没有改变两个节点的通信逻辑。两边仍然靠 `/joint_states` 和 `/joint_torques` 连接。

## 3. 让 launch 文件能被 ROS2 找到

在 `robot_controller/CMakeLists.txt` 的 `ament_package()` 之前加入：

```cmake
install(DIRECTORY launch
  DESTINATION share/${PROJECT_NAME})
```

如果 `package.xml` 中还没有 launch 运行依赖，加入：

```xml
<exec_depend>launch</exec_depend>
<exec_depend>launch_ros</exec_depend>
```

重新构建并 source：

```bash
cd ~/ros2_ws
colcon build --symlink-install \
  --packages-select robot_controller mujoco_bridge
source install/setup.bash
```

现在一条命令启动两端：

```bash
ros2 launch robot_controller sim_control.launch.py
```

按 `Ctrl+C` 时，这次 launch 启动的进程会一起收到退出请求。

## 4. launch 之后仍然要用 graph 检查

“一键启动成功”不等于接口一定接对。

启动后仍然检查：

```bash
ros2 node list
ros2 topic list -t
ros2 topic info /joint_states
ros2 topic info /joint_torques
ros2 param get /controller_node kp
```

这几条命令分别回答：

```text
程序真的启动了吗？
接口名字和类型是什么？
话题两端真的连上了吗？
参数真的以预期值启动了吗？
```

以后排查 ROS2 工程，优先从这种可观察事实开始，而不是先翻几百行源码猜“应该已经连上了”。

## 5. Topic、Service、Parameter 分别在解决什么

这三个东西不应该被硬塞进同一个“通信方式”分类。

### Topic：持续的数据流

适合：

```text
关节状态
关节控制量
传感器数据
周期性估计结果
```

发布者不断产生样本，订阅者按自己的需要接收和处理。

### Service：一次请求对应一次响应

适合这种问题：

```text
“请把仿真重置到初始状态，成功了吗？”
“请保存一次标定结果，保存到哪里了？”
“请求切换到某模式，是否允许切换？”
```

调用者关心的是：**这一次请求对应的结果是什么。**

选择 service 的理由不是“topic 可能丢消息，service 就不会丢”。Topic 自身有 QoS 和可靠性策略；service 的核心区别是交互模型本来就是 request → response。

第四次任务并不需要自己实现 service，所以这里到边界为止，不再塞一份完整 client/server 答案让学生照抄。以后第一次真的需要“发请求并得到对应结果”时，再学习 `create_service` 和 `create_client`。

### Parameter：节点配置

参数解决：

```text
这个节点以什么 kp/kd 启动？
阈值是多少？
某个功能是否开启？
```

它不是控制器和执行端每一周期互相传状态的替代品。

## 6. 还有 Action，但这次先不展开

ROS2 还提供 Action，用于持续一段时间的目标型任务，并支持反馈和取消。导航到某个目标点就是常见例子。

第四次的“周期发送关节状态和力矩”不需要 Action，因此这里只说明它存在，避免形成“ROS2 只有 topic 和 service 两种接口”这种错误结论。真正做到导航或其他长任务时再学习它的完整用法。

## 7. C++ 更适合控制代码，但不等于“保证实时”

C++ 通常比 Python 更适合需要低开销、可控内存和线程行为的高频控制代码，所以本次把控制器放在 C++ 一侧是合理选择。

但：

```text
用了 C++
≠
获得实时性保证
```

真正的实时性质还取决于操作系统调度、线程优先级、内存分配、阻塞操作、中间件、执行器、程序本身的最坏执行时间等。ROS2 提供了构建实时系统所需的能力和设计空间，但语言名字本身不能提供 deadline 保证。

第四次只要求把程序边界和通信链路搭对，不把“硬实时控制”偷偷并入本次学习目标。

## 8. “别人也是 ROS2 节点”还不等于可以直接接上

以后拿一个陌生 ROS2 包来复用时，至少先查：

```text
它有哪些节点和可执行程序？
发布 / 订阅哪些 topic？
消息类型是什么？
字段、单位、坐标系怎样定义？
QoS 是否兼容？
有哪些参数？
需要哪些依赖和 launch 配置？
```

涉及定位、视觉等系统时还会出现时间戳、坐标变换等新的接口约束。

所以 ROS2 提供的是一套**标准化连接机制和观察工具**。它降低了程序集成成本，但没有消灭接口适配本身。

## 9. 第四次培训最终验收

完成第四次以后，不要求背 ROS2 API，也不要求读懂任意陌生 ROS2 工程。应该能够独立完成下面这一条链：

```text
第三次单进程程序
        ↓ 划分职责和接口契约
controller_node + sim_node
        ↓ topic
状态和力矩在两个进程间流动
        ↓ CLI
能观察节点、接口、类型、频率和消息
        ↓ parameters
启动时配置控制器
        ↓ launch
一条命令启动完整仿真控制链
```

最终检查：

```bash
ros2 launch robot_controller sim_control.launch.py
ros2 node list
ros2 topic list -t
ros2 topic info /joint_states
ros2 topic info /joint_torques
ros2 topic hz /joint_states
ros2 param get /controller_node kp
```

如果这些结果和自己的接口表一致，而且控制器与 MuJoCo 已经不存在直接源码调用关系，本次“利用 ROS2 将控制和执行解耦”的目标就完成了。
