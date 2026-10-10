# 从 `unitree_mujoco` 读真实仿真工程

第二次任务最后要求阅读 Unitree 官方 `unitree_mujoco`，再判断自己的仿真程序在结构和线程上还缺什么。这里直接读官方固定版本，不先造一套“理想工程结构”。

使用：

```text
unitreerobotics/unitree_mujoco
commit 1eb6642e3f3fdfb7fb13a9794fd6a2dd93ea0e7d
```

可以检出：

```bash
git clone https://github.com/unitreerobotics/unitree_mujoco.git
cd unitree_mujoco
git checkout 1eb6642e3f3fdfb7fb13a9794fd6a2dd93ea0e7d
```

Python 仿真器主要看三个文件：

```text
simulate_python/
├── unitree_mujoco.py
├── config.py
└── unitree_sdk2py_bridge.py
```

## 为什么官方程序会有两个线程

先打开 `simulate_python/config.py`：

```python
SIMULATE_DT = 0.005
VIEWER_DT = 0.02
```

这两个数对应两个不同周期：

```text
物理仿真：
0.005 s / step
→ 200 Hz

画面刷新：
0.02 s / sync
→ 50 Hz
```

如果仍把它们写在一个循环里：

```python
while viewer.is_running():
    mujoco.mj_step(model, data)
    viewer.sync()
```

两件事就会以同一循环节奏串行执行。

官方入口 `unitree_mujoco.py` 把它们分成两个真实函数：

```text
SimulationThread
PhysicsViewerThread
```

程序末尾：

```python
viewer_thread = Thread(target=PhysicsViewerThread)
sim_thread = Thread(target=SimulationThread)

viewer_thread.start()
sim_thread.start()
```

这里 `Thread(target=...)` 接收的是要在线程中执行的函数，所以传入 `PhysicsViewerThread`、`SimulationThread` 本身，没有括号。

两个函数都以：

```python
while viewer.is_running():
    ...
```

作为循环条件。窗口结束后，两个循环也会退出；这个版本没有另外创建一套 `Event` 停止机制。

## `SimulationThread` 怎样保持 0.005 s 的物理周期

物理线程每轮先记录开始时间：

```python
step_start = time.perf_counter()
```

完成 `mj_step()` 后计算这一轮已经花掉多少时间：

```python
time_until_next_step = (
    mj_model.opt.timestep
    - (time.perf_counter() - step_start)
)
```

如果还有剩余时间：

```python
if time_until_next_step > 0:
    time.sleep(time_until_next_step)
```

而入口前面已经把：

```python
mj_model.opt.timestep = config.SIMULATE_DT
```

设成 0.005 s。

所以这里的关系是：

```text
期望一轮 0.005 s
        ↓
实际 mj_step 等代码已经耗时 x
        ↓
剩余 0.005 - x
        ↓
sleep 剩余时间
```

它不是简单地“每轮 sleep(0.005)”；计算时间本身已经占用了周期的一部分。

## 两个线程为什么要共用一把 `Lock`

入口文件创建：

```python
locker = threading.Lock()
```

物理线程：

```python
locker.acquire()
mujoco.mj_step(mj_model, mj_data)
locker.release()
```

显示线程：

```python
locker.acquire()
viewer.sync()
locker.release()
```

两个函数都操作同一组：

```text
mj_model
mj_data
viewer
```

如果 `mj_step()` 正在更新仿真状态，`viewer.sync()` 同时又来读取这份状态，就会出现并发访问。两边使用**同一把** `locker` 后：

```text
SimulationThread 获得 lock
→ mj_step
→ 释放

PhysicsViewerThread
必须等上面的临界区结束
→ 获得同一把 lock
→ viewer.sync
→ 释放
```

这里需要学习的是“为什么这两处必须共享同一个锁”，而不是背一个孤立的 `Lock` API。

## 电机命令究竟在哪里进入 MuJoCo

`SimulationThread()` 里创建：

```python
unitree = UnitreeSdk2Bridge(mj_model, mj_data)
```

这个类定义在：

```text
simulate_python/unitree_sdk2py_bridge.py
```

其中订阅低层命令：

```python
self.low_cmd_suber.Init(self.LowCmdHandler, 10)
```

收到 `LowCmd` 后进入：

```python
def LowCmdHandler(self, msg):
    ...
```

核心循环最终把每个电机的命令写进：

```python
self.mj_data.ctrl[i] = (...)
```

所以命令真正走的是：

```text
LowCmd
  ↓
LowCmdHandler
  ↓
mj_data.ctrl[i]
  ↓
SimulationThread
  ↓
mujoco.mj_step(...)
```

这条链和前面自己写的仿真程序是同一种结构：外部命令最终都必须落到 `data.ctrl`，下一次 `mj_step()` 才会使用它。

## MuJoCo 状态又怎样变回 `LowState`

同一个 `UnitreeSdk2Bridge` 还创建周期线程调用：

```text
PublishLowState
```

其中电机状态来自：

```python
self.mj_data.sensordata[...]
```

分别写入：

```python
self.low_state.motor_state[i].q
self.low_state.motor_state[i].dq
self.low_state.motor_state[i].tau_est
```

最后：

```python
self.low_state_puber.Write(self.low_state)
```

所以反馈方向是：

```text
MuJoCo sensordata
        ↓
PublishLowState
        ↓
LowState
        ↓
low_state_puber.Write(...)
```

把两个方向合起来：

```text
控制程序 / Unitree SDK
        │
        │ LowCmd
        ↓
LowCmdHandler
        ↓
mj_data.ctrl
        ↓
SimulationThread
        ↓
mj_step
        ↓
mj_data.sensordata
        ↓
PublishLowState
        ↓
LowState
```

这里每一个箭头都能在固定版本源码中落到一个实际函数或数据成员。

## 源码追踪练习

练习文件：

```text
starter/03_unitree_trace/trace.json
```

需要自己从源码中填出：

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

其中：

- `entry_file`：Python 仿真器入口；
- `config_file`：定义场景路径和两个周期的配置文件；
- `simulation_thread`：真正调用 `mj_step()` 的函数；
- `viewer_thread`：调用 `viewer.sync()` 的函数；
- `command_path`：从低层命令处理到 `mj_data.ctrl`、再到物理推进；
- `state_path`：从 `mj_data.sensordata` 到 `LowState` 发布。

名称使用源码原名。完成后：

```bash
cd 第二次培训/MuJoCo/starter/03_unitree_trace
bash test.sh
```

通过时得到：

```text
[PASS] unitree_mujoco source trace
```

## 回到自己的机器狗程序

现在再看第二次自己写的程序，是否值得修改已经有了具体参照。

如果自己的程序仍然只有：

```text
mj_step
→ viewer.sync
```

而且一个循环已经能满足任务，就没有理由为了“像工程项目”而强行加线程。

如果确实需要让物理推进和显示以不同周期运行，可以借用刚才已经追清的结构：

```text
两个真实的长期循环
→ 两个 Thread

两个循环访问同一份 MjData
→ 同一把 Lock

周期不同
→ 分别按 SIMULATE_DT / VIEWER_DT 控制节奏
```

配置文件也不是因为“工程项目应该有 config”才拆出来；当场景路径、仿真周期、显示周期等值需要被多个模块共同使用时，像官方 `config.py` 那样集中保存才开始产生实际作用。
