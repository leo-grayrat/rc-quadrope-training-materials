# 参数、launch 与服务

现在已经有两个可以独立运行的节点：

```text
sim_node
  ↓ /joint_states
controller_node
  ↓ /joint_torques
sim_node
```

最后处理两件实际使用时马上会遇到的问题：控制参数怎样在启动时配置，以及两个节点怎样一起启动。

## 1. 用参数配置控制器

以 `kp`、`kd` 为例，在 `ControllerNode` 构造函数中声明并读取：

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

控制计算使用 `kp_`、`kd_`。启动时可以覆盖默认值：

```bash
ros2 run robot_controller controller_node \
  --ros-args \
  -p kp:=30.0 \
  -p kd:=2.0
```

`--ros-args` 表示后面的选项交给 ROS2 解析；`-p name:=value` 在启动时覆盖对应参数，所以这里把 `kp`、`kd` 改成 30.0 和 2.0。

查看当前参数：

```bash
ros2 param list /controller_node
ros2 param get /controller_node kp
```

这里的实现只在构造函数中把参数读入成员变量，因此本次把参数作为 **启动配置** 使用。

## 2. 用 launch 启动完整控制链

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

`LaunchDescription` 保存这次启动要执行的一组动作；`launch_ros.actions.Node` 表示“启动一个 ROS2 节点”这个动作。`launch_ros` 是 launch 系统中面向 ROS2 节点的部分。

在 `CMakeLists.txt` 的 `ament_package()` 之前安装 launch 文件：

```cmake
install(DIRECTORY launch
  DESTINATION share/${PROJECT_NAME})
```

`package.xml` 加入运行依赖：

```xml
<exec_depend>launch</exec_depend>
<exec_depend>launch_ros</exec_depend>
<exec_depend>mujoco_bridge</exec_depend>
```

`exec_depend` 表示运行这个包时需要存在的依赖；这里 launch 文件运行时要用到 launch 系统和 `mujoco_bridge`。

重新构建：

```bash
cd ~/ros2_ws
colcon build --symlink-install \
  --packages-select robot_controller mujoco_bridge
source install/setup.bash
```

以后可以直接启动：

```bash
ros2 launch robot_controller sim_control.launch.py
```

启动后继续从 ROS2 图检查实际结果：

```bash
ros2 node list
ros2 topic list -t
ros2 topic info /joint_states
ros2 topic info /joint_torques
ros2 topic hz /joint_states
ros2 param get /controller_node kp
```

这些结果应与前面写下的接口契约一致。

## 3. Topic、Service、Parameter 的使用位置

第四次已经实际使用了两类接口：

- **Topic**：持续流动的关节状态和关节力矩；
- **Parameter**：`kp`、`kd` 这类节点配置。

Service 对应一次请求和一次响应，例如：

```text
请求重置仿真 → 返回是否成功
请求保存标定 → 返回保存结果
请求切换某模式 → 返回是否允许
```

### 最终验收

第四次最终应能独立完成：

```text
第三次单进程仿真控制程序
        ↓
controller_node + sim_node
        ↓
/joint_states 与 /joint_torques
        ↓
C++ 控制数学与状态机
        ↓
参数配置
        ↓
launch 一起启动
```

最后运行：

```bash
ros2 launch robot_controller sim_control.launch.py
ros2 node list
ros2 topic list -t
ros2 topic info /joint_states
ros2 topic info /joint_torques
ros2 topic hz /joint_states
ros2 param get /controller_node kp
```

检查两个节点、两条话题、参数和消息频率都与自己的接口表一致，并确认 `controller_node` 与 `sim_node` 之间不存在直接源码调用关系。
