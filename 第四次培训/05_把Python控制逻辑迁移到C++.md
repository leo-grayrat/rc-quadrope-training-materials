# 把第三次控制逻辑从 Python 迁移到 C++

前一篇已经把 MuJoCo 留在 `sim_node`，把控制器留在 `controller_node`。C++ 控制器现在要复现第三次 Python 版本已经建立的两段计算：

```text
当前 q / dq + 目标
→ pd_torque
→ tau

起点 + 终点 + 时间
→ linear_target
→ q_des / dq_des
```

“迁移成功”的判据也很直接：**给 Python 和 C++ 同一组输入，两边必须得到同一结果。**

## 先钉住一个具体的 PD 输入

第三次的 `pd_torque()` 定义是：

```math
\tau
=
\tau_{ff}
+K_p(q_{des}-q)
+K_d(\dot q_{des}-\dot q),
```

最后再把结果限制在 `[-tau_limit, tau_limit]`。

例如固定：

```text
q         = 0.5
dq        = 0.1
q_des     = 0.0
dq_des    = 0.0
kp        = 20.0
kd        = 1.0
tau_ff    = 0.0
tau_limit = 100.0
```

第三次已经能算出：

```math
\tau
=20(0-0.5)+1(0-0.1)
=-10.1.
```

C++ 版接口是：

```cpp
double pd_torque(
    double q,
    double dq,
    double q_des,
    double dq_des,
    double kp,
    double kd,
    double tau_ff,
    double tau_limit);
```

因此下面这次调用：

```cpp
pd_torque(0.5, 0.1, 0.0, 0.0, 20.0, 1.0, 0.0, 100.0)
```

也必须返回 `-10.1`。

这就是从 Python 迁移到 C++ 时最小的一条等价关系：

```text
同一组 q / dq / target / gain
        ↓
Python pd_torque
        = 
C++ pd_torque
        ↓
同一个 tau
```

练习目录已经给出函数声明和测试：

```text
第四次培训/starter/01_control_math_cpp/
```

补全 `control_math.cpp` 后运行：

```bash
cd 第四次培训/starter/01_control_math_cpp
bash test.sh
```

除了刚才的普通 PD，测试还会给纯阻尼和正负两侧的限幅输入。

## 轨迹函数也用同一组数对齐

第三次的站立轨迹从：

```text
q_start
→ q_target
```

在 `duration` 时间内线性移动。

C++ 接口是：

```cpp
struct Target
{
  double q_des;
  double dq_des;
};

Target linear_target(
    double q_start,
    double q_target,
    double elapsed,
    double duration);
```

例如：

```text
q_start = 0
q_target = 1
duration = 2 s
elapsed = 1 s
```

这时正好走到一半，所以：

```text
q_des  = 0.5
dq_des = 0.5
```

同一个 starter 的测试会继续检查：

```text
t = 0       → 起点
t = 1       → 中点
t = 2       → 终点，dq_des = 0
t > 2       → 保持终点
1 → -1      → 反方向速度应为负
```

迁移时要保住第三次已经建立的函数关系：

```text
输入
→ 数学关系
→ 输出
```

语言换了，关系不能换。

## 把一个 `JointState` 关节接到这两个函数

4.3 中已经能从 ROS2 消息读：

```cpp
const double q = msg->position[...];
const double dq = msg->velocity[...];
```

现在选 `joint_order` 中的一个关节，例如 `FL_hip_joint`。

消息里的数组顺序不保证就是 `joint_order`，所以先用：

```text
msg->name
→ 找到 "FL_hip_joint" 在消息中的 index
→ 用同一个 index 读取 position / velocity
```

得到这个关节的 `q / dq` 后，控制链已经能完整写成：

```text
JointState 中的 FL_hip_joint
        ↓
q / dq
        ↓
当前控制状态
        ↓
q_des / dq_des
        ↓
pd_torque(...)
        ↓
tau
```

阻尼状态没有位置目标：

```text
kp = 0
dq_des = 0
tau_ff = 0
```

所以：

```math
\tau=-K_d\dot q.
```

站立状态则需要第三次已经实现的线性目标：

```text
切换到 Standing 的瞬间
→ 保存这个关节的 q_start

之后每次收到新状态
→ elapsed = now - stand_start_time
→ linear_target(q_start, q_stand, elapsed, duration)
→ q_des / dq_des
→ pd_torque(...)
```

这里真正需要新增的状态只有“切换瞬间必须保存一次”的两类量：

```text
每个关节的 q_start
统一的 stand_start_time
```

如果每轮都把当前 `q` 重新写进 `q_start`，轨迹起点就会不断移动，第三次的线性目标关系会被破坏。

## 从一个关节扩到 12 个关节

单关节链正确以后，再把它放进 `joint_order`。

对每条 `JointState`，先建立：

```text
name
→ position / velocity 的 index
```

随后按固定的 `joint_order` 逐个取：

```text
FL_hip_joint
FL_thigh_joint
FL_calf_joint
...
```

每个名字经过同一条控制链：

```text
name
→ q / dq
→ 当前状态机目标
→ pd_torque
→ tau
```

最终把 12 个 `tau` 按 `joint_order` 放进：

```cpp
std_msgs::msg::Float64MultiArray out;
```

于是两端的数组约定仍然是：

```text
out.data[0]
↔ joint_order[0]

out.data[1]
↔ joint_order[1]

...
```

这正是上一篇 `sim_node` 用来把力矩重新写回 actuator 的顺序。

## 接回 `controller_node`

控制器回调最终只做四件事：

```text
JointState
    ↓
按名字恢复 joint_order 下的 12 个 q / dq
    ↓
根据 Damping / Standing 生成目标
    ↓
对每个关节调用 pd_torque
    ↓
发布 12 个 tau
```

MuJoCo 节点不需要改控制数学；它仍然发布状态、接收力矩并写入 `data.ctrl`。

### 验收

先运行纯函数测试：

```bash
cd 第四次培训/starter/01_control_math_cpp
bash test.sh
```

然后手工发布一条固定的 `JointState`。对其中一个关节使用前面的简单输入，例如：

```text
q = 0.5
dq = 0.1
q_des = 0
dq_des = 0
kp = 20
kd = 1
```

对应位置的输出力矩应为：

```text
-10.1
```

单个关节能和手算对上以后，再检查 12 个输出是否按 `joint_order` 排列。最后接回 `sim_node`，同一组固定状态下，C++ 控制器应与第三次 Python 控制器产生相同的目标和力矩（允许正常浮点误差）。
