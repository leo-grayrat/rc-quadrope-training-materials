# 把第三次控制逻辑从 Python 迁移到 C++

前一篇已经把 ROS2 通信和 MuJoCo 执行端单独跑通。现在只处理控制器内部的变化：把第三次 Python 里的控制数学和状态机迁移到 C++。

顺序固定为：

```text
纯控制函数
    ↓
12 关节映射与状态机
    ↓
接回 controller_node
```

## 1. 先迁移纯控制函数

第三次控制器中最容易独立验证的是两个纯函数：

```text
pd_torque
linear_target
```

练习目录：

```text
第四次培训/starter/01_control_math_cpp/
```

其中已经给定函数接口，补全 `control_math.cpp` 后运行：

```bash
cd 第四次培训/starter/01_control_math_cpp
bash test.sh
```

### PD

接口：

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

沿用第三次定义：

```math
\tau = K_p(q_{des}-q)+K_d(\dot q_{des}-\dot q)+\tau_{ff}
```

并把结果限制在：

```math
[-\tau_{limit},\tau_{limit}]
```

测试覆盖普通 PD、纯阻尼以及正负两侧限幅。

### 线性目标轨迹

接口：

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

仍然使用第三次的轨迹：

```math
s=\mathrm{clip}\left(\frac{t}{T},0,1\right)
```

```math
q_{des}=(1-s)q_{start}+s q_{target}
```

运动结束前：

```math
\dot q_{des}=\frac{q_{target}-q_{start}}{T}
```

结束后：

```math
q_{des}=q_{target},\qquad \dot q_{des}=0
```

测试覆盖起点、中点、终点、超过终点和反方向运动。这些输入与第三次 Python 练习检查的是同一组性质。

## 2. 恢复 12 关节映射和状态机

纯函数通过以后，把 `/joint_states` 转成控制器内部的 12 关节状态：

```text
JointState
   ↓ name → index
按 joint_order 取 q / dq
   ↓
状态机
   ↓
目标轨迹
   ↓
pd_torque
   ↓
按 joint_order 得到 12 个 tau
```

阻尼和站立仍然使用第三次的两个状态：

```cpp
enum class ControlState
{
  Damping,
  Standing,
};
```

切换到 `Standing` 时保存一次：

```text
q_start
stand_start_time
```

随后每次收到新的 `JointState`：

```text
Damping
  → kp = 0
  → dq_des = 0
  → pd_torque

Standing
  → elapsed = now - stand_start_time
  → linear_target
  → pd_torque
```

同一个控制器内部使用同一种时间来源计算 `elapsed`。

这里可以分层检查错误来源：单关节结果错，检查纯函数；单关节正确而 12 关节错，检查名字映射和 `joint_order`；12 个控制量都正确以后，再检查 ROS2 和 MuJoCo 两端。

## 3. 接回 `controller_node`

最后把状态机放进 `/joint_states` 的回调：

```text
/joint_states
      ↓
12 个 q / dq
      ↓
状态机 + linear_target + pd_torque
      ↓
12 个 tau
      ↓
/joint_torques
```

MuJoCo 节点仍然只负责上一篇已经完成的状态发布和力矩执行。

### 检查点

**产物**：C++ 版 `pd_torque`、`linear_target` 和两状态控制逻辑，并接入 `controller_node`。

**给定边界**：

- 数学定义沿用第三次；
- 关节顺序沿用 `joint_order`；
- 话题名和消息类型沿用前两篇；
- 状态切换逻辑沿用第三次的阻尼/站立要求。

**判定方式**：

1. `starter/01_control_math_cpp/test.sh` 全部通过；
2. 手工发送一条 12 关节 `JointState` 后，`/joint_torques` 恰好有 12 个元素；
3. 对同一组固定状态和参数，C++ 控制器与第三次 Python 版本输出的目标和力矩在浮点误差范围内一致；
4. 接回 `sim_node` 后，MuJoCo 一侧的话题接口不需要修改。

纯函数、关节映射、ROS2 联调分别有自己的检查信号，出现问题时可以按层定位。
