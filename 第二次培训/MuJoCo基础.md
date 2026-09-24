# MuJoCo 与机器人仿真基础

这份讲义配合第二次培训的 `c++python概念-任务.pdf` 使用。**任务单原文是要求本身，本文不改任务，也不提供一份可以直接提交的完整机器狗程序。**

这一阶段需要解决的不是“把 MuJoCo 的所有 API 记住”，而是先建立一条最小仿真链路：

```text
模型文件 -> MuJoCo 编译模型 -> 运行状态 -> 写入控制量 -> 推进一步 -> 查看新的状态
```

完成以后，你应该能自己把一份机器人模型放进平坦场景，知道关节状态和执行器控制量在哪里，能解释零力矩意味着什么，并能继续阅读 `unitree_mujoco` 一类真实项目。

> 本文以 MuJoCo 3.x 的官方 Python 接口为主。MuJoCo 仍在更新，接口细节以官方文档为准：https://mujoco.readthedocs.io/

---

## 1. MuJoCo 在程序里扮演什么角色

MuJoCo 是刚体动力学与接触仿真器。对于四足机器人，它根据模型中的质量、惯量、关节、碰撞几何和执行器等信息，在给定控制输入后计算下一时刻的状态。

最小控制循环可以理解为：

```text
读取当前状态
    ↓
控制程序计算控制量
    ↓
写入 data.ctrl
    ↓
mj_step(model, data)
    ↓
得到下一时刻 qpos / qvel / ...
```

后续强化学习策略也只是“控制程序”的一种：神经网络读取观测，输出动作；MuJoCo 仍然负责物理状态怎样变化。

### MjModel 与 MjData

Python 中最常见的两行是：

```python
model = mujoco.MjModel.from_xml_path("scene.xml")
data = mujoco.MjData(model)
```

`MjModel` 保存编译后的**模型结构和参数**，例如关节、执行器、质量、时间步长等；`MjData` 保存仿真过程中不断变化的**当前状态和计算结果**。

先记住这个区分：

```text
model：这个系统是什么
 data：这个系统现在处于什么状态
```

---

## 2. 先跑一个和机器狗无关的小模型

在处理 URDF、mesh 和十几个关节以前，先用一个方块确认 MuJoCo 环境和基本调用链路。

创建 `scene.xml`：

```xml
<mujoco model="falling_box">
    <option timestep="0.002" gravity="0 0 -9.81"/>

    <worldbody>
        <light pos="0 0 3"/>
        <geom name="floor" type="plane" size="5 5 0.1"/>

        <body name="box" pos="0 0 1">
            <freejoint/>
            <geom type="box" size="0.1 0.1 0.1" mass="1"/>
        </body>
    </worldbody>
</mujoco>
```

创建 `simulate.py`：

```python
import time

import mujoco
import mujoco.viewer

model = mujoco.MjModel.from_xml_path("scene.xml")
data = mujoco.MjData(model)

with mujoco.viewer.launch_passive(model, data) as viewer:
    while viewer.is_running():
        start = time.time()

        mujoco.mj_step(model, data)
        viewer.sync()

        remain = model.opt.timestep - (time.time() - start)
        if remain > 0:
            time.sleep(remain)
```

运行：

```bash
python3 simulate.py
```

如果方块在重力作用下落到地面，说明至少下面几件事已经通了：

- Python 能 import MuJoCo；
- XML 能成功编译；
- `MjData` 能创建；
- `mj_step()` 在推进物理；
- Viewer 能显示当前状态。

这段代码是理解 MuJoCo 的小实验，不是第二次任务的机器狗答案。

---

## 3. MJCF 中最先要看懂的四类东西

MuJoCo 原生模型格式是 MJCF。它是 XML，但不要把注意力放在 XML 语法本身，先看模型表达了什么。

一个最简结构可能是：

```xml
<mujoco model="example">
    <option/>
    <asset/>
    <worldbody/>
    <actuator/>
</mujoco>
```

### body：刚体与坐标系

`body` 通过嵌套形成运动学树。子 body 的位姿相对父 body 定义。

### joint：允许怎样相对运动

例如：

```xml
<joint name="hip" type="hinge" axis="0 1 0"/>
```

表示当前 body 相对父 body 有一个绕指定轴旋转的自由度。

如果一个 body 没有 joint，它默认与父 body 固连。

### geom：形状、碰撞与可视几何

`geom` 可以是 box、sphere、capsule、mesh 等。它既可能参与碰撞，也可能用于显示和惯量推断，具体由属性决定。

### inertial：质量与惯量

动力学仿真不仅需要“长什么样”，还需要“有多重、质量怎样分布”。很多模型会显式给出 inertial；有时 MuJoCo 也会从 geom 推断。

如果模型外观看起来正常但一仿真就猛烈抖动、翻飞，质量、惯量、碰撞形状和初始穿模都值得检查。

---

## 4. 自由基座为什么让 qpos 看起来不像“关节数”

四足机器人机身通常不是固定在世界坐标系，而是可以在三维空间自由移动。MJCF 中常见：

```xml
<freejoint/>
```

自由基座具有三维平移和三维转动自由度，但姿态通常用四元数存储，因此它在 `qpos` 中占 7 个数，在 `qvel` 中占 6 个数。

所以：

```text
model.nq 不一定等于 model.nv
```

这是第一次读四足 MuJoCo 代码时很容易困惑的地方。

可以先打印：

```python
print("nq =", model.nq)
print("nv =", model.nv)
print("nu =", model.nu)
print("qpos =", data.qpos)
print("qvel =", data.qvel)
print("ctrl =", data.ctrl)
```

其中：

| 量 | 先这样理解 |
|---|---|
| `nq` | 广义位置数组长度 |
| `nv` | 广义速度数组长度 |
| `nu` | 控制输入维度，通常与执行器数量有关 |
| `qpos` | 当前广义位置 |
| `qvel` | 当前广义速度 |
| `ctrl` | 当前执行器控制输入 |

不要在没确认索引含义之前直接假设 `qpos[0]` 就是第一个腿关节。

---

## 5. joint 与 actuator 不是一回事

`joint` 描述机械结构允许怎样运动；`actuator` 描述控制输入怎样作用到模型上。

只有 joint 并不意味着 `data.ctrl` 中自动出现一个控制量。需要在 `actuator` 中定义执行器。

本次任务要求使用**力矩模式**。在 MuJoCo 中，最直接的基础形式可以使用 `motor` actuator，例如：

```xml
<actuator>
    <motor name="hip_motor" joint="hip" gear="1"/>
</actuator>
```

对于这种简单设置，可以把 `data.ctrl[i]` 理解为作用到对应自由度上的直接驱动力/力矩控制输入；`gear` 等参数会影响控制量到广义力的映射。

因此：

```python
data.ctrl[:] = 0.0
```

表示不给这些 actuator 主动输出控制力矩。

**零力矩不等于“强制保持姿势不动”。** 在重力和接触作用下，如果初始姿态没有支撑好，机器人仍然可能落下、趴倒或滑动。这一点对任务非常重要。

---

## 6. URDF 与 MJCF：任务要求和软件能力分开理解

人工任务单要求：从 URDF 出发，**自行转换为 MJCF**，再在 MuJoCo 中完成场景与控制设置。这个要求保持不变。

同时需要知道一个技术事实：当前 MuJoCo 本身也支持解析 URDF；官方文档甚至提供了 URDF 扩展。但 URDF 只能表达 MuJoCo 模型能力的一个子集，场景、执行器、MuJoCo 特有参数等通常仍需要在 MJCF 中继续整理。

因此这次任务采用：

```text
已有 URDF
   ↓
检查 mesh 与关节结构
   ↓
转换 / 保存为 MJCF
   ↓
在 MJCF 中补场景、执行器、初始状态等
   ↓
Python 加载并运行
```

这不是因为“MuJoCo 完全不能打开 URDF”，而是因为最终目标是得到一份可以继续修改、适合后续仿真的 MJCF。

### 转换后先检查什么

不要转换完就立刻写控制程序。先确认：

- mesh 文件路径是否还能找到；
- 机器人各 link/body 是否都存在；
- 关节名字、轴方向和范围是否合理；
- base 是否需要自由基座；
- 质量与惯量有没有明显异常；
- 模型是否在 Viewer 中以正确姿态显示。

如果模型本身就加载不正确，后面改 Python 控制代码没有意义。

---

## 7. mesh 路径是最常见的模型问题之一

机器人模型通常引用 STL 等 mesh。路径可能来自原 URDF 的目录结构，转换后如果 XML 与 mesh 的相对位置改变，就会出现找不到资源的错误。

MJCF 可以通过 `compiler` 指定 mesh 目录，例如：

```xml
<compiler meshdir="meshes"/>
```

或者在 `<mesh file="..."/>` 中使用正确的相对路径。

排查时不要只看 XML 中“写了什么”，还要从主 MJCF 文件所在位置计算实际相对路径。

可以先在 Shell 中：

```bash
pwd
find . -maxdepth 3 -type f | head
```

确认文件真实存在。

---

## 8. 场景与机器人模型最好分开

任务要求加入平坦地面。长期看，不建议把“机器人本体”和“这个实验使用的世界”完全揉成一个文件。

可以理解成：

```text
robot.xml   -> 机器人本体
scene.xml   -> 地面、灯光、机器人放在哪
```

MJCF 可以通过 `<include>` 或更现代的模型组合机制复用模型。初学阶段不必把组合系统做得很复杂，但至少要知道：

> 地形是场景的一部分，不是机器狗身体的一部分。

最简单的平面通常类似：

```xml
<geom name="floor" type="plane" size="5 5 0.1"/>
```

但机器狗“放在地面上”还涉及基座高度与各关节初始角度。

---

## 9. “静止趴在地面上”真正需要你决定什么

人工任务的完成条件是让机器狗在平坦场景中静止趴着，并要求关节输出力矩为 0。

这里至少有三个独立问题。

### 初始基座位姿

如果 base 初始位置太高，零力矩时它一定先落下；太低则可能一开始就与地面严重穿透。

### 初始关节角

`data.ctrl[:] = 0` 不会自动把腿摆成趴下姿势。关节初始角要来自你的模型结构和目标姿态。

### 接触后是否稳定

即使初始看起来是趴着，接触几何重叠、惯量异常或者摩擦设置不合适，也可能导致抖动或滑动。

因此“静止”不要只理解为截图看着差不多。运行一段时间后观察：

```python
print(data.time)
print(data.qpos)
print(data.qvel)
```

如果速度持续很大，说明它实际上没有稳定下来。

任务没有要求你写一个通用判定器，但验收时应能解释自己怎样判断它已经稳定。

---

## 10. 初始状态放在哪里

模型默认状态可以写进 MJCF，也可以在 Python 创建 `MjData` 后修改。

例如程序结构可以是：

```python
model = mujoco.MjModel.from_xml_path("scene.xml")
data = mujoco.MjData(model)

# TODO: 根据自己模型的 qpos 结构设置初始基座和关节姿态
# data.qpos[...] = ...

mujoco.mj_forward(model, data)
```

`mj_forward()` 在不推进时间的情况下，根据当前 `qpos`、`qvel` 等重新计算依赖状态。手工修改初始状态后调用它，便于在开始 stepping 前得到一致的几何和动力学量。

也可以在 MJCF 使用 keyframe 保存一组姿态。两种方式都可以，关键是你知道初始状态来自哪里，而不是在代码和 XML 中重复设置后忘记哪一个最终生效。

---

## 11. 第二次任务的 Python 程序可以先按这个骨架思考

下面不是可提交的完整答案，只把仿真程序的职责分开：

```python
import time

import mujoco
import mujoco.viewer

# 1. 加载你整理好的场景 / MJCF
model = mujoco.MjModel.from_xml_path("...")
data = mujoco.MjData(model)

# 2. TODO: 设置自己机器人的初始姿态

# 3. 本任务要求零力矩
# TODO: 确认 ctrl 的维度和执行器含义后设置

mujoco.mj_forward(model, data)

with mujoco.viewer.launch_passive(model, data) as viewer:
    while viewer.is_running():
        start = time.time()

        # 4. TODO: 如有需要，在这里维持 / 更新控制输入

        mujoco.mj_step(model, data)
        viewer.sync()

        remain = model.opt.timestep - (time.time() - start)
        if remain > 0:
            time.sleep(remain)
```

真正需要你自己补的是：

- 加载哪个文件；
- 你的 `qpos` 各段分别对应什么；
- 哪组关节角是合理的趴卧初始姿态；
- 模型里有几个 actuator；
- 怎样确认所有关节控制确实为零；
- 运行后怎样判断姿态稳定。

这些信息不能从一份通用讲义替你猜出来，因为它们取决于具体机器人模型。

---

## 12. 仿真步和控制周期不是同一个概念

`mujoco.mj_step(model, data)` 每次推进一个物理时间步，步长来自：

```xml
<option timestep="0.002"/>
```

这表示一次物理步对应 0.002 s，也就是 500 Hz 的仿真更新频率。

以后控制器可能只在 50 Hz 更新一次，那么可以保持同一组控制量执行多次 `mj_step()`：

```text
计算一次控制量
    ↓
物理 step 多次
    ↓
再次计算控制量
```

本次零力矩任务不需要复杂控制器，但现在区分这两个概念，后面做 RL policy decimation 时会直接用到。

---

## 13. 怎样阅读 `unitree_mujoco` 一类项目

人工任务要求在基础程序完成后阅读 `unitree_mujoco` 并改进自己的程序结构。不要一上来从仓库第一行读到最后一行。

先找四件事。

### 模型从哪里进入程序

搜索：

```text
MjModel
from_xml_path
.xml
scene
```

### 仿真主循环在哪里

搜索：

```text
mj_step
while
viewer
```

### 控制量在哪里写入

搜索：

```text
ctrl
actuator
motor
```

### 状态在哪里读取

搜索：

```text
qpos
qvel
sensor
```

先画出：

```text
初始化 -> 读取状态 -> 控制 -> step -> 显示 / 通信
```

再去理解线程、通信类和工程封装。否则很容易只看到“项目用了很多类和线程”，却不知道它们围绕哪一条仿真链路工作。

### 关于线程

任务让你思考线程设计是否足够优秀，并不意味着“线程越多越好”。

先问：

- 哪些工作必须以不同频率运行；
- 哪些操作会阻塞；
- 多线程后共享的 `data`、控制量和状态由谁同步；
- 单线程是否已经足够完成当前任务。

能说清楚“为什么需要线程”比机械把 viewer、control、simulation 各拆一个线程更重要。

---

## 14. 常见问题的排查顺序

### import 失败

```text
ModuleNotFoundError: No module named 'mujoco'
```

先确认：

```bash
which python3
python3 -m pip show mujoco
```

不要只凭“我刚刚 pip install 过”判断环境一致。

### XML 编译失败

先读错误里指出的文件、元素和行号；再检查 XML 结构、属性名和引用路径。

### mesh 找不到

检查主 XML 所在目录、`meshdir` 和实际 mesh 路径。

### 一开始就弹飞

优先检查：

- 初始碰撞几何是否严重重叠；
- base 高度是否合理；
- 质量与惯量是否异常；
- 时间步是否过大；
- 自己是否误写了很大的 `ctrl`。

### `ctrl` 写了但关节不动

检查：

- `model.nu` 是否大于 0；
- actuator 是否真的绑定到目标 joint；
- 控制索引是否对应正确 actuator；
- 力矩量级、gear、限制是否合理；
- 关节是否被锁死或已经顶到 limit。

### 零力矩后机器人倒下

这不一定是 MuJoCo 错误。零力矩意味着不主动驱动关节，不意味着保持初始姿态。回到第 9 节检查初始姿态与接触是否能在重力下自然稳定。

---

## 15. 选做 C++ 时，概念并不会变

Python 与 C++ API 写法不同，但核心对象仍然是：

```text
model
data
ctrl
step
```

因此选做 C++ 版本时，不要把任务理解成“重新学一遍 MuJoCo”。先让 Python 版本的模型、状态索引和控制关系完全清楚，再把同一条逻辑迁移到 C++。

---

## 16. 完成本阶段后应该能说明

验收时至少应能结合自己的模型回答：

- `MjModel` 与 `MjData` 分别存什么；
- `qpos`、`qvel`、`ctrl` 在自己的机器人中大致对应哪些量；
- 为什么自由基座会让 `nq != nv`；
- `joint` 和 `actuator` 的区别；
- 本次为什么使用 motor actuator，以及零 `ctrl` 表示什么；
- 为什么零力矩不保证机器人自动保持站立或趴卧；
- 自己从 URDF 到 MJCF 做了哪些处理；
- mesh 路径、平地场景和初始姿态分别在哪里定义；
- 自己怎样确认机器人已经稳定；
- `unitree_mujoco` 中模型加载、状态读取、控制和 stepping 分别在哪里；
- 如果引入线程，具体是为了解决什么问题。

能够在自己的代码和 XML 上指出这些内容，比背 MuJoCo API 名称更重要。

---

## 17. 官方资料

建议优先查官方文档：

- Modeling / URDF：https://mujoco.readthedocs.io/en/latest/modeling.html
- XML Reference：https://mujoco.readthedocs.io/en/latest/XMLreference.html
- Python：https://mujoco.readthedocs.io/en/latest/python.html
- Programming / Simulation：https://mujoco.readthedocs.io/en/latest/programming/simulation.html

遇到模型字段或 actuator 属性不确定时，直接查 XML Reference，通常比搜索二手博客更可靠。
