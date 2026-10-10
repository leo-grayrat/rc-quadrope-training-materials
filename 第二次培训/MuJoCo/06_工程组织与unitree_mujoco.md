# 工程组织与 unitree_mujoco

机器狗程序跑通以后，代码开始同时承担几类不同工作：模型与场景配置、初始状态、物理推进、画面刷新，以及以后会加入的控制和通信。`unitree_mujoco` 正好是一份可以对照的真实工程：它把这些工作拆开，并让仿真和显示以不同频率运行。

这一部分先把多文件和并发运行连成一个完整问题，再去追 `unitree_mujoco` 的真实数据流。

## 从一个能运行的脚本到多个并发职责

最开始的四足仿真完全可以只有一个文件：

```text
加载场景
→ 设置初始 qpos
→ 写 ctrl
→ mj_step
→ viewer.sync
```

随着程序增长，先按已有职责拆文件。例如：

```text
quadruped_sim/
├── robot.py
├── simulator.py
└── main.py
```

`robot.py` 可以保存与机器人状态直接相关的函数：

```python
import mujoco


def set_joint_position(model, data, name, position):
    joint_id = mujoco.mj_name2id(
        model,
        mujoco.mjtObj.mjOBJ_JOINT,
        name,
    )
    qpos_adr = model.jnt_qposadr[joint_id]
    data.qpos[qpos_adr] = position
```

`simulator.py` 负责仿真循环：

```python
import mujoco
import mujoco.viewer


def run(model, data):
    with mujoco.viewer.launch_passive(model, data) as viewer:
        while viewer.is_running():
            mujoco.mj_step(model, data)
            viewer.sync()
```

`main.py` 只负责组合：

```python
import mujoco

from robot import set_joint_position
from simulator import run


model = mujoco.MjModel.from_xml_path("scenes/flat_scene.xml")
data = mujoco.MjData(model)

set_joint_position(model, data, "FL_hip_joint", 0.3)
mujoco.mj_forward(model, data)
data.ctrl[:] = 0.0

run(model, data)
```

拆文件没有改变数据关系：`model` 和 `data` 仍然在入口中创建，再通过函数参数传给其它模块。程序只是把不同职责从一个文件中分开。

### 当仿真和显示不再使用同一个循环

MuJoCo 的物理步长可能是：

```text
0.005 s
```

也就是 200 Hz；画面刷新没有必要达到 200 Hz，例如 50 Hz 已经足够流畅。于是程序开始出现两个长期运行的任务：

```text
物理仿真：200 Hz
显示刷新： 50 Hz
```

Python 可以用 `Thread` 让两个函数分别运行：

```python
from threading import Thread

sim_thread = Thread(target=simulation_loop)
viewer_thread = Thread(target=viewer_loop)

sim_thread.start()
viewer_thread.start()
```

`target` 需要传函数本身，因此这里写 `simulation_loop`，没有括号。

长期运行的线程还需要一个明确的停止信号。标准库中的 `Event` 可以承担这个职责：

```python
from threading import Event, Thread

stop_event = Event()


def worker():
    while not stop_event.is_set():
        ...

thread = Thread(target=worker)
thread.start()

# 需要结束时
stop_event.set()
thread.join()
```

`set()` 发出停止信号，`join()` 等待线程真正退出。

### 两个线程访问同一份 MuJoCo 状态

如果一个线程正在：

```python
mujoco.mj_step(model, data)
```

另一个线程同时：

```python
viewer.sync()
```

它们会碰到同一份仿真状态。需要保证一段操作执行期间不被另一个线程同时进入时，可以用同一把 `Lock`：

```python
from threading import Lock

lock = Lock()

with lock:
    mujoco.mj_step(model, data)
```

显示线程也使用同一个 `lock`：

```python
with lock:
    viewer.sync()
```

同一时刻只能有一个线程进入这两个临界区。

到这里已经足够解释当前程序为什么会用到 `Thread`、`Event` 和 `Lock`：

```text
仿真和显示要以不同周期长期运行
→ Thread

两个长期循环需要能一起结束
→ Event

两个线程会同时访问同一份 MjData
→ Lock
```

这些工具都由当前四足仿真的实际结构引出来。

## 源码追踪任务：自己走一遍 `unitree_mujoco`

这次源码阅读固定到 Unitree 官方仓库的这个版本：

```text
unitreerobotics/unitree_mujoco
commit 1eb6642e3f3fdfb7fb13a9794fd6a2dd93ea0e7d
```

可以直接：

```bash
git clone https://github.com/unitreerobotics/unitree_mujoco.git
cd unitree_mujoco
git checkout 1eb6642e3f3fdfb7fb13a9794fd6a2dd93ea0e7d
```

仓库中的 Python 仿真器位于 `simulate_python/`。正式读参考分析之前，先自己沿源码找出两条完整路径：

```text
低层电机命令
→ 写入 MuJoCo 控制输入
→ 物理推进

MuJoCo 传感器状态
→ 组装 LowState
→ 发布出去
```

练习文件在：

```text
starter/03_unitree_trace/trace.json
```

其中固定了六个字段：

```json
{
  "entry_file": "",
  "config_file": "",
  "simulation_thread": "",
  "viewer_thread": "",
  "command_path": [],
  "state_path": []
}
```

填写规则：

- `entry_file`：Python 仿真器入口文件，相对仓库根目录；
- `config_file`：保存机器人场景和仿真/显示周期的配置文件；
- `simulation_thread`：真正调用 `mj_step()` 的线程函数名；
- `viewer_thread`：调用 `viewer.sync()` 的线程函数名；
- `command_path`：按顺序写出“接收低层命令的处理函数 → MuJoCo 控制数组 → 物理推进函数”；
- `state_path`：按顺序写出“MuJoCo 传感器数组 → 组装低层状态的函数 → 最终发布调用”。

路径中的名称使用源码里的原名，不翻译。

填写以后运行：

```bash
cd 第二次培训/MuJoCo/starter/03_unitree_trace
bash test.sh
```

通过时会得到：

```text
[PASS] unitree_mujoco source trace
```

这里评测的是源码中确实存在的文件、函数和数据路径。测试没有要求你评价“这个架构好不好”，只检查是否真正把关键链路追到了具体代码。

## 参考：官方 Python 仿真器怎样连起来

下面对应刚才的源码追踪任务，可以在跑过 `test.sh` 以后对照。

入口是：

```text
simulate_python/unitree_mujoco.py
```

配置来自：

```text
simulate_python/config.py
```

当前固定版本中：

```python
SIMULATE_DT = 0.005
VIEWER_DT = 0.02
```

因此物理仿真的目标周期是 0.005 s，显示线程每隔约 0.02 s 刷新一次。

入口文件加载场景并创建：

```text
MjModel
MjData
Viewer
```

同时创建一把全局 `threading.Lock`。随后定义两个顶层线程函数：

```text
SimulationThread
PhysicsViewerThread
```

`SimulationThread` 的核心循环可以概括为：

```text
记录本轮开始时间
→ 获得 locker
→ mujoco.mj_step(mj_model, mj_data)
→ 释放 locker
→ 按剩余 timestep 睡眠
```

`PhysicsViewerThread` 则是：

```text
获得同一把 locker
→ viewer.sync()
→ 释放 locker
→ sleep(VIEWER_DT)
```

因此这把锁保护的是仿真线程和显示线程共同访问的 MuJoCo 状态；两个线程的周期又分别来自 `SIMULATE_DT` 和 `VIEWER_DT`。

### SDK 命令怎样进入 `data.ctrl`

`SimulationThread` 中会创建：

```text
UnitreeSdk2Bridge(mj_model, mj_data)
```

它定义在：

```text
simulate_python/unitree_sdk2py_bridge.py
```

低层命令订阅器把消息交给：

```text
LowCmdHandler
```

这个处理函数逐个电机读取命令中的前馈力矩、目标位置、目标速度、`kp`、`kd`，并根据 MuJoCo 传感器中的当前状态计算控制量，最后写进：

```text
mj_data.ctrl[i]
```

随后 `SimulationThread` 的 `mujoco.mj_step(...)` 使用这些控制输入推进下一步物理状态。

所以命令方向可以压成：

```text
LowCmd
↓
LowCmdHandler
↓
mj_data.ctrl
↓
mujoco.mj_step
```

### MuJoCo 状态怎样重新发布出去

桥接类还创建了一个周期线程来调用：

```text
PublishLowState
```

它从：

```text
mj_data.sensordata
```

读取电机位置、速度、估计力矩以及其它传感器信息，写入 `low_state`，最后调用：

```text
low_state_puber.Write(self.low_state)
```

因此状态方向是：

```text
mj_data.sensordata
↓
PublishLowState
↓
low_state_puber.Write
```

把两个方向合起来以后，官方仿真器的关键数据流就是：

```text
控制程序 / Unitree SDK
          │
          │ LowCmd
          ↓
   UnitreeSdk2Bridge
          ↓
      mj_data.ctrl
          ↓
    SimulationThread
          ↓
        mj_step
          ↓
   MuJoCo sensor data
          ↓
   UnitreeSdk2Bridge
          ↓
       LowState
```

这条链比“看 README、找入口、看看多线程”更具体：每一个箭头都能落到固定版本中的文件、函数和数据成员。

### 回到自己的程序

自己的四足仿真是否需要照搬官方结构，要看实际职责。

如果目前只有一个物理循环和一个显示窗口，单线程程序足够清楚；为了“工程化”而强行加入线程没有收益。

当程序真的出现不同频率的长期任务时，可以逐项迁移已经看过的结构：

```text
配置越来越多
→ 把路径和周期集中到配置模块

仿真与显示需要不同周期
→ 分成独立循环

两个循环直接访问同一份 MjData
→ 用同一把 Lock 保护临界区

```

如果要练习模块化，可以把已经跑通的机器狗程序拆成两个或更多 Python 模块，但拆分前后的行为必须保持一致：仍加载同一场景、使用同一初始姿态、`ctrl` 保持零输入，上一章的模型检查器仍应通过。

这一阶段不把“必须多线程”作为验收条件。源码追踪任务的 `test.sh` 通过，说明已经把官方项目最关键的线程和控制/状态数据流定位到真实代码；自己的程序是否继续拆线程，再由实际需求决定。
