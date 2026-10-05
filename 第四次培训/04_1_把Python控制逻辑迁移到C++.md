# 把第三次控制逻辑从 Python 迁移到 C++

前一篇已经把程序在进程层面拆开：MuJoCo 执行端负责读取状态、执行力矩，控制器负责状态机、目标轨迹和 PD。

这里还有一个独立问题不能藏在“把控制逻辑搬过去”这句话里：**第三次的完整控制逻辑原本是 Python，而第四次最终希望控制器使用 C++。**

这件事和 ROS2 通信本身没有必然关系。最稳妥的顺序是把两个变化分开验证：

```text
先证明 ROS2 接口能传对数据
        ↓
再证明 C++ 控制数学和原 Python 一致
        ↓
最后把完整状态机接回 ROS2 节点
```

前一篇已经完成第一步。这一篇只处理第二、三步。

## 1. 先固定“搬什么”，不要整文件翻译

第三次程序中真正属于控制器的核心可以继续拆成三层：

```text
纯计算函数
- pd_torque
- linear_target
        ↓
状态机
- DAMPING
- STANDING
- 切换时保存 q_start / t0
        ↓
ROS2 适配
- JointState → q / dq
- tau → Float64MultiArray
```

迁移时先处理纯计算函数。它们没有 ROS2、MuJoCo、Viewer 和 SDK 依赖，输入一样就应该得到一样的输出，因此最容易独立检查。

不要直接把第三次几百行 Python 从上到下逐句翻译成 C++。那样一旦最后机器人行为不对，你无法判断问题来自：

```text
数学公式
状态机
关节顺序
ROS2 消息
还是仿真执行端
```

## 2. 先迁移 PD，并用固定输入验算

C++ 里先约定一个纯函数接口：

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

它仍然实现第三次已经学过的：

```math
\tau = K_p(q_{des}-q)+K_d(\dot q_{des}-\dot q)+\tau_{ff}
```

最后把结果限制在：

```math
[-\tau_{limit},\tau_{limit}]
```

先不要把它塞进 ROS2 回调。用一个临时 `main()` 或小测试程序跑固定输入。

### 检查 1：普通 PD

```text
q = 0.5
dq = 0.1
q_des = 0
dq_des = 0
kp = 20
kd = 1
tau_ff = 0
tau_limit = 100
```

应该得到：

```text
-10.1
```

### 检查 2：纯阻尼

```text
q = 任意值
dq = 3
q_des = 任意值
dq_des = 0
kp = 0
kd = 2
tau_ff = 0
tau_limit = 100
```

应该得到：

```text
-6
```

### 检查 3：正向限幅

```text
q = 0
q_des = 10
kp = 100
kd = 0
tau_ff = 0
tau_limit = 5
```

未经限幅的结果远大于 5，最终应该得到：

```text
5
```

再自行构造一个负向限幅输入，确认结果为 `-5`。

这些检查和第三次 `starter/01_pd_control` 验收的是同一组性质。C++ 版本先通过这些固定输入，再接回 ROS2。

## 3. 再迁移线性目标轨迹

给轨迹函数一个不依赖容器和 ROS2 的接口。单关节可以先写成：

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

第三次的定义保持不变：

```math
s=\mathrm{clip}\left(\frac{t}{T},0,1\right)
```

```math
q_{des}=(1-s)q_{start}+s q_{target}
```

运动尚未结束时：

```math
\dot q_{des}=\frac{q_{target}-q_{start}}{T}
```

结束后：

```math
q_{des}=q_{target},\qquad \dot q_{des}=0
```

用下面几个点检查：

| `q_start` | `q_target` | `elapsed` | `duration` | 预期 `q_des` | 预期 `dq_des` |
|---:|---:|---:|---:|---:|---:|
| 0 | 1 | 0 | 2 | 0 | 0.5 |
| 0 | 1 | 1 | 2 | 0.5 | 0.5 |
| 0 | 1 | 2 | 2 | 1 | 0 |
| 0 | 1 | 3 | 2 | 1 | 0 |
| 1 | -1 | 1 | 2 | 0 | -1 |

这几项分别检查起点、中点、终点、超过终点和反方向运动，与第三次 `starter/04_trajectory` 的边界一致。

## 4. 12 个关节只是重复调用，不要重新发明公式

单关节函数通过后，再在控制器节点中按 `joint_order` 对 12 个关节逐个调用：

```text
JointState
   ↓ 根据 name 建立索引
按 joint_order 取 q / dq
   ↓
每个关节生成 q_des / dq_des
   ↓
pd_torque
   ↓
按 joint_order 得到 12 个 tau
```

这里新增的是“数组和关节名怎样对应”，不是新的控制理论。

因此出现问题时也可以分层检查：

- 单关节函数错：回到固定输入测试；
- 单关节对、12 关节错：检查 `name → index → joint_order` 映射；
- 12 个数都对、仿真仍不对：再检查 ROS2 接口和执行端。

## 5. 最后迁移状态机

阻尼和站立两个状态本身不需要因为换成 C++ 而重新设计。

可以保留同样的状态：

```cpp
enum class ControlState
{
  Damping,
  Standing,
};
```

切换到站立的瞬间仍然要保存：

```text
q_start
stand_start_time
```

随后每次收到新的 `JointState`：

```text
当前状态
  ↓
Damping
  → kp = 0
  → dq_des = 0
  → 阻尼力矩

Standing
  → elapsed = now - stand_start_time
  → linear_target
  → pd_torque
```

这里的 `now` 可以使用节点时钟，但本次只要求同一控制器内部用一致的时间来源。不要一边用 ROS2 时间、一边又拿系统墙钟做 `elapsed`。

## 6. 接回 ROS2 前先做一次“纯控制”检查

在真正启动 `controller_node` 之前，至少保证：

```text
pd_torque 固定输入正确
linear_target 五个边界点正确
12 关节映射能按 joint_order 产出 12 个结果
状态切到 Standing 时只保存一次 q_start / t0
```

前三项都可以脱离 ROS2 检查。状态机可以用几帧手工构造的状态依次调用，确认切换逻辑。

只有这些确定性部分通过以后，再把它们放进 `/joint_states` 的回调中。

## 7. 最终回到第四篇的数据流

到这里，完整控制器才变成：

```text
/joint_states
      ↓
名字映射得到 12 个 q / dq
      ↓
状态机
      ↓
linear_target
      ↓
pd_torque
      ↓
12 个 tau
      ↓
/joint_torques
```

MuJoCo 节点完全不需要知道这些力矩是由 Python 还是 C++ 算出来的。它只认已经约定好的话题和数据含义。

这正好给“接口解耦”一个可以直接验证的结果：**控制器内部语言和实现发生了变化，执行端接口没有跟着重写。**

## 8. 本篇练习契约

**产物**：C++ 版 `pd_torque`、`linear_target` 和两状态控制逻辑，并接入上一节的 `controller_node`。

**边界**：

- 数学定义沿用第三次，不重新设计控制算法；
- 关节顺序沿用 `joint_order`；
- ROS2 话题名和消息类型沿用前一篇；
- 先检查纯函数，再接 ROS2；
- 本题不引入模板元编程、Eigen 重构、多线程等额外工程内容。

**判定方式**：

1. `pd_torque` 通过本篇四类固定输入；
2. `linear_target` 通过表格中的五个边界点；
3. 手工发送一条 12 关节 `JointState` 后，`/joint_torques` 恰好有 12 个元素；
4. 对同一组固定状态和参数，C++ 控制器与第三次 Python 版本输出的关节目标和力矩在浮点误差范围内一致；
5. 换回 MuJoCo 执行端后，不需要修改 `sim_node` 的话题接口。

这样，跨语言迁移本身有独立反馈，不会和 ROS2 通信问题揉成一个大黑箱。
