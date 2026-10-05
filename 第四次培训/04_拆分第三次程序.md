# 把第三次程序真正拆开

前一篇已经证明：一个 C++ 控制器节点可以接收 `JointState`，并发布关节力矩。现在把第三次已有的 MuJoCo 程序接到另一端。

这一篇的目标不是重新写仿真。**保留第三次已经能工作的模型加载、关节索引、Viewer、`mj_step()` 等代码，只把“读取状态”和“写入力矩”包成 ROS2 接口。**

最终结构是：

```text
┌──────────────────────┐
│ controller_node C++  │
│ 状态机 / 轨迹 / PD   │
└──────────┬───────────┘
           │
   /joint_torques
           ↓
         ROS2
           ↑
    /joint_states
           │
┌──────────┴───────────┐
│ sim_node Python      │
│ MuJoCo / Viewer      │
│ data.qpos/qvel/ctrl  │
└──────────────────────┘
```

这里让两种语言同时存在是有原因的：第三次的 MuJoCo 程序本来就是 Python，而控制器接下来希望用 C++ 组织。ROS2 的消息接口把语言边界隔开，不要求为了“统一语言”先重写一个已经工作的仿真程序。

## 1. 给现有 MuJoCo 程序加一个 Python ROS2 包

先创建包：

```bash
cd ~/ros2_ws/src
ros2 pkg create \
  --build-type ament_python \
  mujoco_bridge \
  --dependencies rclpy sensor_msgs std_msgs
```

把第三次仿真程序复制到：

```text
mujoco_bridge/mujoco_bridge/sim_node.py
```

不要一上来重写内部算法。先保证这个文件在脱离 ROS2 改动时，仍然能看出第三次程序原来的几部分：

```text
加载模型
建立 joints / joint_order
初始化状态
进入 Viewer 循环
读取 qpos / qvel
写 data.ctrl
mj_step
```

## 2. 给执行端加发布者和订阅者

在 `sim_node.py` 中加入：

```python
import rclpy
from rclpy.node import Node
from sensor_msgs.msg import JointState
from std_msgs.msg import Float64MultiArray
```

建立一个很薄的桥接节点：

```python
class MujocoBridge(Node):
    def __init__(self):
        super().__init__('sim_node')

        self.state_pub = self.create_publisher(
            JointState, '/joint_states', 10)

        self.torque_sub = self.create_subscription(
            Float64MultiArray,
            '/joint_torques',
            self.torque_callback,
            10,
        )

        self.latest_tau = None

    def torque_callback(self, msg):
        self.latest_tau = list(msg.data)
```

这个类暂时不认识 MuJoCo。它只负责 ROS2 一侧的事情：

```text
发布 JointState
接收 Float64MultiArray
保存最近一次 tau
```

## 3. 把 MuJoCo 状态转成 `JointState`

给类增加一个方法。这里直接复用第三次程序已有的 `joint_order` 和 `joints`：

```python
def publish_state(self, data, joint_order, joints):
    msg = JointState()
    msg.header.stamp = self.get_clock().now().to_msg()
    msg.name = list(joint_order)
    msg.position = [
        float(data.qpos[joints[name]['qpos_adr']])
        for name in joint_order
    ]
    msg.velocity = [
        float(data.qvel[joints[name]['qvel_adr']])
        for name in joint_order
    ]
    self.state_pub.publish(msg)
```

这里建立的是非常具体的一条边：

```text
MuJoCo qpos/qvel address
        ↓ 按 joint_order 读取
JointState.name/position/velocity
        ↓
/joint_states
```

位置和速度仍然是第三次使用的关节侧量，单位分别为 rad 和 rad/s。

## 4. 把力矩消息写回 `data.ctrl`

再增加：

```python
def apply_latest_torque(self, data, joint_order, joints):
    if self.latest_tau is None:
        return

    if len(self.latest_tau) != len(joint_order):
        return

    for name, tau in zip(joint_order, self.latest_tau):
        actuator_id = joints[name]['actuator_id']
        data.ctrl[actuator_id] = tau
```

第一版如果还没有收到控制器命令，就保持初始化时的零控制量。收到长度不对的消息时也不要猜测应该怎样补齐。

进入实体电机之前，还需要给“多久没有新命令就进入安全状态”定义明确策略；那属于实机安全契约，本篇先在仿真中把数据流接通，不偷偷替任务规定一个超时时间。

## 5. 在仿真循环里处理 ROS2 回调

启动 ROS2 节点：

```python
rclpy.init()
node = MujocoBridge()
```

然后把第三次原来的循环改成这个顺序：

```python
while viewer.is_running():
    node.publish_state(data, joint_order, joints)

    rclpy.spin_once(node, timeout_sec=0.0)
    node.apply_latest_torque(data, joint_order, joints)

    mujoco.mj_step(model, data)
    viewer.sync()
```

退出时：

```python
node.destroy_node()
rclpy.shutdown()
```

这里故意没有为了“工程化”先加线程和锁。

当前循环本来就持续运行；`rclpy.spin_once(..., timeout_sec=0.0)` 可以在每轮循环中处理一次当前可用的 ROS2 工作。这样先把：

```text
仿真循环
+ ROS2 回调
```

放进同一条执行链，学生可以直接看见数据什么时候被读取、回调什么时候被处理、力矩什么时候写回 MuJoCo。

只有当以后真正发现某些回调阻塞仿真、不同任务需要独立频率等问题时，再引入多线程执行器、锁或其他同步设计。

## 6. 注册 Python 可执行程序

打开 `mujoco_bridge/setup.py`，在 `console_scripts` 中加入：

```python
entry_points={
    'console_scripts': [
        'sim_node = mujoco_bridge.sim_node:main',
    ],
},
```

因此 `sim_node.py` 需要把原来的启动逻辑放进：

```python
def main(args=None):
    ...
```

如果原程序已经有 `main()`，就直接在原结构上加入 `rclpy.init()`、桥接节点和清理逻辑，不要为了符合某个模板把整个仿真重写一遍。

## 7. 第一次完整联调

重新构建：

```bash
cd ~/ros2_ws
colcon build --symlink-install \
  --packages-select robot_controller mujoco_bridge
source install/setup.bash
```

先开仿真执行端：

```bash
ros2 run mujoco_bridge sim_node
```

另开终端启动控制器：

```bash
ros2 run robot_controller controller_node
```

第三个终端检查 ROS2 图：

```bash
ros2 node list
ros2 topic list -t
ros2 topic info /joint_states
ros2 topic info /joint_torques
ros2 topic hz /joint_states
ros2 topic hz /joint_torques
```

至少应该看到：

```text
/controller_node
/sim_node
```

并且：

```text
/joint_states
  publisher:  sim_node
  subscriber: controller_node

/joint_torques
  publisher:  controller_node
  subscriber: sim_node
```

## 8. 再把第三次完整控制逻辑搬进控制器

到这一步之前，`controller_node` 只需要能完成一个很简单的 PD 计算。连接关系确认正确以后，再迁移第三次程序中真正属于“控制器”的部分：

```text
当前状态 q / dq
      ↓
阻尼 / 站立状态
      ↓
状态切换时保存 q_start / t0
      ↓
linear_target
      ↓
pd_torque
      ↓
12 个 tau
```

下面这些继续留在 `sim_node`：

```text
MjModel / MjData
joint / actuator address
Viewer
mj_step
将 tau 写入 data.ctrl
```

判断一段代码应该去哪边时，可以问：

> 如果明天把 MuJoCo 换成真实机器人，这段代码是否仍然应该原样存在？

站立轨迹和 PD 通常仍然属于控制器；`data.ctrl`、Viewer 和 `mj_step()` 显然属于仿真执行端。

## 9. 本篇练习契约

**产物**：两个可以独立启动的程序：

```text
robot_controller/controller_node
mujoco_bridge/sim_node
```

它们之间只通过 ROS2 话题交换状态和关节侧力矩。

**边界**：

- 第三次已经工作的 MuJoCo 建模、关节索引和控制算法可以直接复用；
- 状态消息类型固定为 `sensor_msgs/msg/JointState`；
- 力矩消息类型固定为 `std_msgs/msg/Float64MultiArray`；
- 本题不要求自定义消息、多线程、QoS 调优或实机通信；
- 不允许让 `controller_node` 重新直接读取 MuJoCo 的 `data`，否则拆分没有完成。

**判定方式**：

1. 两个包分别能构建并被 `ros2 run` 启动；
2. `ros2 node list` 同时出现两个节点；
3. `ros2 topic info` 能看到两条话题两端各有对应发布者/订阅者；
4. `ros2 topic echo /joint_states --once` 能看到 12 个关节名、位置和速度；
5. `ros2 topic echo /joint_torques --once` 能看到 12 个力矩；
6. 停掉 `controller_node` 后，`sim_node` 本身仍能继续运行，而不会因为 import 或直接函数调用依赖控制器源码而退出。

前五项证明通信和数据契约成立；最后一项证明两边在进程层面确实已经拆开。
