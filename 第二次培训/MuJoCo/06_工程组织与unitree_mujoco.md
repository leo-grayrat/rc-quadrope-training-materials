# 工程组织与 unitree_mujoco

前面的机器狗程序已经能运行。人工任务还要求继续阅读 `unitree_mujoco`，考虑程序结构和线程设计。

这一部分先把 Python 多文件和多线程真正写一遍，再去读开源项目。否则“模块”“仿真线程”“通信线程”只会停留在名词上。

## 1. 从自己的单文件程序开始拆

机器狗程序最开始完全可以都写在 `simulate.py` 中：

```text
加载模型
设置初始 qpos
创建 Viewer
写 ctrl
mj_step
显示
```

程序继续增长以后，可以把不同职责拆开。

先看三个文件的最小结构：

```text
quadruped_sim/
├── robot.py
├── simulator.py
└── main.py
```

### robot.py：处理机器人状态

把“根据 joint 名称设置初始角”单独写成函数：

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

这里没有新的 MuJoCo API，只是把上一章反复执行的三步放进函数。

以后可以继续在这个文件中加入：

- 设置整组初始关节角；
- 根据名字读取关节状态；
- 保存机器人 joint 名称。

### simulator.py：负责推进仿真

```python
import mujoco
import mujoco.viewer


def run(model, data):
    with mujoco.viewer.launch_passive(model, data) as viewer:
        while viewer.is_running():
            mujoco.mj_step(model, data)
            viewer.sync()
```

这个文件集中负责“仿真怎样运行”。

### main.py：把各部分组合起来

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

这里的 `FL_hip_joint` 只是示例，实际名称使用自己的模型。

现在多文件之间的关系很明确：

```text
main.py
├── 调用 robot.py 处理机器人状态
└── 调用 simulator.py 运行 MuJoCo
```

`import` 以后，函数仍然通过参数接收 `model`、`data`。它们没有因为拆文件就自动变成“全局共享对象”。

### 练习：先只拆文件

把已经跑通的机器狗程序拆成至少两个 Python 文件。

拆分前后要求保持：

- 加载同一份场景；
- 初始姿态相同；
- `data.ctrl[:] = 0.0`；
- Viewer 中运行结果相同。

这一步只改变代码组织，不增加线程。

## 2. Python 线程最基本怎么写

Python 标准库中的 `threading` 可以启动线程。

先看一个完全独立于 MuJoCo 的最小程序：

```python
from threading import Thread


def worker():
    print("worker finished")


thread = Thread(target=worker)
thread.start()
thread.join()

print("main finished")
```

关键有三步。

创建线程：

```python
thread = Thread(target=worker)
```

`target=worker` 表示线程启动后执行 `worker` 函数。这里传的是函数本身，所以没有写 `worker()`。

启动：

```python
thread.start()
```

等待线程结束：

```python
thread.join()
```

因为主线程在 `join()` 后才继续，所以最后一定会看到：

```text
worker finished
main finished
```

## 3. 一个持续运行的线程怎样停下来

仿真、通信、日志线程往往不是执行一次就结束，而是持续循环。

可以用 `Event` 给线程一个停止信号：

```python
import time
from threading import Event, Thread

stop_event = Event()


def worker():
    while not stop_event.is_set():
        print("working")
        time.sleep(0.1)
```

启动：

```python
thread = Thread(target=worker)
thread.start()
```

需要停止时：

```python
stop_event.set()
thread.join()
```

`set()` 改变 Event 的状态，工作线程下一次检查 `is_set()` 时退出循环；`join()` 再等待它真正结束。

这个写法比直接强行结束线程更容易管理清理过程。

## 4. 两个线程碰同一份数据时要考虑同步

假设一个线程正在执行：

```python
mujoco.mj_step(model, data)
```

另一个线程同时执行：

```python
viewer.sync()
```

两边都在使用同一份 `data`。如果某段操作要求数据在执行过程中保持一致，就需要控制谁可以同时进入这段代码。

Python 可以使用 `Lock`：

```python
import threading

lock = threading.Lock()
```

进入临界区：

```python
with lock:
    mujoco.mj_step(model, data)
```

另一个线程也使用同一把锁：

```python
with lock:
    viewer.sync()
```

同一时刻只能有一个线程持有这把锁。

因此可以形成：

```text
SimulationThread
    ↓
拿 lock
    ↓
mj_step(model, data)
    ↓
释放 lock

PhysicsViewerThread
    ↓
拿同一把 lock
    ↓
viewer.sync()
    ↓
释放 lock
```

`with lock:` 会在代码块结束时自动释放锁。也可以手工调用 `acquire()` 和 `release()`，但必须保证所有路径都能释放，否则其它线程可能一直等待。

## 5. 线程之间传数据：Queue

另一个常见需求是：仿真线程产生状态，日志或通信线程读取这些状态。

这时不一定要让两个线程都直接操作 `MjData`。可以传一份状态副本。

Python 标准库提供线程安全的队列：

```python
from queue import Queue

state_queue = Queue()
```

仿真线程中：

```python
state_queue.put(data.qpos.copy())
```

这里使用 `.copy()`，放进队列的是当时的状态快照。后面 `data.qpos` 再变化，不会把已经放进队列的这份数组一起改掉。

另一个线程读取：

```python
state = state_queue.get()
print(state)
```

所以一种更清楚的数据关系可以是：

```text
仿真线程
  │
  │ qpos.copy()
  ↓
Queue
  ↓
日志线程
```

实际程序如果仿真产生数据远快于日志消费，还要考虑降低发送频率或限制队列长度；这里先掌握线程间传递状态的基本方法。

## 6. 为什么仿真和 Viewer 可能使用不同频率

MuJoCo 的物理步长决定仿真更新频率。

例如：

```text
timestep = 0.005 s
```

就是每秒 200 个物理步。

Viewer 没必要每个物理步都刷新。如果：

```text
VIEWER_DT = 0.02 s
```

就是 50 Hz 显示。

于是可以出现：

```text
仿真：200 Hz
显示： 50 Hz
```

它们具有不同循环周期，这就是把两项工作分开的一个实际理由。

控制、通信也可能有自己的更新频率。阅读项目时，看到多个循环或线程，需要继续找各自的时间间隔，而不是只记住“这里用了多线程”。

## 7. 现在真正读 unitree_mujoco

这里使用 Unitree 官方仓库：

<https://github.com/unitreerobotics/unitree_mujoco>

仓库同时有 C++ 和 Python 仿真器。当前学习 Python，重点看：

```text
simulate_python/
├── config.py
├── unitree_mujoco.py
└── unitree_sdk2py_bridge.py
```

官方中文 README 中给出的 Python 启动方式是：

```bash
cd simulate_python
python3 unitree_mujoco.py
```

所以入口已经很明确：

```text
simulate_python/unitree_mujoco.py
```

下面直接沿这个文件往下读。

## 8. config.py：先看程序依赖哪些配置

`simulate_python/config.py` 中保存了机器人、场景和时间参数。

当前文件中可以看到：

```text
ROBOT
ROBOT_SCENE
SIMULATE_DT
VIEWER_DT
```

其中 `ROBOT_SCENE` 决定加载哪份场景。

当前版本的配置中：

```text
SIMULATE_DT = 0.005
VIEWER_DT   = 0.02
```

也就是前面刚算过的 200 Hz 物理步和 50 Hz Viewer。

这时已经能预期：主程序里很可能存在两个不同频率的循环。

## 9. unitree_mujoco.py：先找到初始化

文件开头创建了一把全局 `threading.Lock`，然后加载：

```text
MjModel
MjData
Viewer
```

场景路径来自刚才看到的 `config.ROBOT_SCENE`。

所以目前调用关系是：

```text
config.py
   │
   │ ROBOT_SCENE
   ↓
unitree_mujoco.py
   ↓
MjModel + MjData + Viewer
```

这和自己的 `main.py` 已经很接近，只是配置被单独放进了 `config.py`。

## 10. unitree_mujoco.py：两个顶层线程到底干什么

这个文件定义了两个主要线程函数：

```text
SimulationThread
PhysicsViewerThread
```

### SimulationThread

它的循环中会：

1. 记录这一物理步开始的时间；
2. 获取 `locker`；
3. 调用 `mujoco.mj_step(mj_model, mj_data)`；
4. 释放 `locker`；
5. 根据 `timestep` 睡眠剩余时间。

所以它的职责很明确：**推进物理仿真，并尽量按照 `SIMULATE_DT` 的节奏运行。**

### PhysicsViewerThread

它的循环中会：

1. 获取同一把 `locker`；
2. 调用 `viewer.sync()`；
3. 释放 `locker`；
4. 按 `VIEWER_DT` 睡眠。

所以它负责显示。

前面学的 `Thread`、`Lock` 和“不同频率”现在都能在真实项目里找到对应位置。

主程序最后创建并启动：

```text
Thread(target=PhysicsViewerThread)
Thread(target=SimulationThread)
```

这就是 Python 线程语法在这个项目里的实际使用。

## 11. 为什么这里需要同一把 Lock

`SimulationThread` 会修改 `mj_data`；`PhysicsViewerThread` 要读取同一份仿真状态并显示。

官方代码用同一个 `locker` 把：

```text
mj_step
```

和：

```text
viewer.sync
```

包在互斥区间中。

因此分析线程设计时，不需要停留在“它用了锁”。可以继续回答：

```text
锁保护谁？
→ mj_model / mj_data / Viewer 相关的共享仿真状态

哪两个地方竞争？
→ SimulationThread 和 PhysicsViewerThread

不加这把锁会失去什么保证？
→ Viewer 可能在仿真状态正在更新时同时读取相关数据
```

这才是锁在当前项目里的实际作用。

## 12. unitree_sdk2py_bridge.py：控制输入从哪里进入 MuJoCo

`SimulationThread` 中还会创建：

```text
UnitreeSdk2Bridge(mj_model, mj_data)
```

它定义在：

```text
simulate_python/unitree_sdk2py_bridge.py
```

这个类把 Unitree SDK 的通信数据和 MuJoCo 状态连接起来。

其中一个关键函数是：

```text
LowCmdHandler
```

收到低层电机命令以后，它最终会写入：

```text
mj_data.ctrl[i]
```

于是控制方向是：

```text
Unitree LowCmd
      ↓
LowCmdHandler
      ↓
mj_data.ctrl
      ↓
SimulationThread 中的 mj_step
```

另一个方向，`PublishLowState` 会读取 MuJoCo 的传感器数据，再通过 SDK 发布机器人状态。

因此整个 Python 仿真器的主数据流可以整理成：

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
  MuJoCo 状态 / sensor
       ↙          ↘
ViewerThread     UnitreeSdk2Bridge
viewer.sync      发布 LowState 等状态
```

这张图已经比“README → 入口 → 主循环”多了一层：每一条箭头都能在具体文件和函数里找到。

## 13. 自己在仓库里重新走一遍

现在不要直接照上面的结论回答，自己从官方仓库重新定位一次。

依次找出：

1. Python 仿真器的启动命令；
2. `ROBOT_SCENE` 在哪个文件设置；
3. `MjModel` 和 `MjData` 在哪里创建；
4. 哪个函数调用 `mj_step()`；
5. 哪个函数调用 `viewer.sync()`；
6. 两个顶层线程在哪里创建；
7. 它们共用的 `Lock` 在哪里创建；
8. `LowCmdHandler` 在哪个文件；
9. 它最终把电机命令写到哪里；
10. `SIMULATE_DT` 和 `VIEWER_DT` 分别是多少。

这十项都能在 `readme_zh.md`、`simulate_python/config.py`、`simulate_python/unitree_mujoco.py` 和 `simulate_python/unitree_sdk2py_bridge.py` 中直接找到。

## 14. 再回到自己的程序

读完以后，至少可以对自己的程序做一次有根据的选择。

例如：

- 把机器人和场景路径移到独立 `config.py`；
- 把机器人状态处理和仿真循环拆到不同模块；
- 如果增加日志线程，用 `Queue` 传递 `qpos.copy()` 等状态副本；
- 如果真的让多个线程访问同一份 MuJoCo 状态，明确哪些操作需要 `Lock`；
- 如果仿真和 Viewer 使用不同刷新频率，分别定义它们的周期。

线程不是验收必须项。简单程序保持单线程完全可以。这里需要掌握的是：**能写出 Python 线程，知道共享数据怎样同步，并能解释 `unitree_mujoco` 为什么这样组织。**
