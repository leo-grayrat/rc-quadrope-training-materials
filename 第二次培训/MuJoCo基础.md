# MuJoCo 与机器人仿真基础

这份讲义配合第二次培训的 [任务.md](./任务.md) 使用。任务要求以任务文件为准，这里补充完成任务需要的 MuJoCo 基础。

仿真程序可以先看成下面这条链路：

```text
模型文件
  ↓
MjModel
  ↓
MjData
  ↓
写入控制量
  ↓
mj_step()
  ↓
更新后的状态
```

后面的内容都围绕这几个对象展开。

> 以下示例使用 MuJoCo 3.x 的 Python 接口。接口细节可以查官方文档：https://mujoco.readthedocs.io/

## 安装与环境检查

先确认当前 Python 和 pip：

```bash
python3 --version
python3 -m pip --version
```

安装：

```bash
python3 -m pip install mujoco
```

检查安装结果：

```bash
python3 -c "import mujoco; print(mujoco.__version__)"
```

如果出现 `ModuleNotFoundError`，继续看：

```bash
which python3
python3 -m pip show mujoco
```

这两条命令可以确认运行脚本时使用的 Python，以及 MuJoCo 安装到了哪个 Python 环境。

项目多起来以后可以再使用 venv 或 Conda 管理不同环境。

## MjModel 与 MjData

加载 MJCF：

```python
model = mujoco.MjModel.from_xml_path("scene.xml")
data = mujoco.MjData(model)
```

`MjModel` 保存编译后的模型信息，例如：

- body、joint、geom；
- 质量和惯量；
- actuator；
- 时间步长；
- 其他模型参数。

`MjData` 保存运行过程中的状态和计算结果，例如：

- `qpos`；
- `qvel`；
- `ctrl`；
- 当前仿真时间；
- 接触和传感器结果。

`MjModel` 创建后基本保持不变，`MjData` 会随着仿真步不断更新。

## 一个最小模型

先用一个方块熟悉加载、step 和 Viewer。

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

也可以直接打开模型：

```bash
python3 -m mujoco.viewer --mjcf=scene.xml
```

前者适合自己控制仿真循环，后者适合快速检查模型。

如果方块能落到地面，说明模型已经成功加载，Viewer 可以工作，`mj_step()` 也在正常推进状态。

## MJCF 的基本结构

MJCF 是 MuJoCo 原生 XML 模型格式。常见顶层元素包括：

```xml
<mujoco model="example">
    <compiler/>
    <option/>

    <asset>
        ...
    </asset>

    <worldbody>
        ...
    </worldbody>

    <actuator>
        ...
    </actuator>

    <sensor>
        ...
    </sensor>
</mujoco>
```

常见作用：

| 元素 | 用途 |
|---|---|
| `compiler` | 模型解析和资源目录等设置 |
| `option` | 时间步长、重力等仿真参数 |
| `asset` | mesh、材质、纹理等资源 |
| `worldbody` | 世界和机器人刚体结构 |
| `actuator` | 执行器 |
| `sensor` | 仿真传感器 |

### body

`body` 表示刚体，也形成坐标系层级。子 body 的位置和姿态相对父 body 定义。

### joint

`joint` 描述 body 相对父 body 的可运动自由度：

```xml
<joint name="hip" type="hinge" axis="0 1 0"/>
```

这里定义了一个绕给定轴旋转的关节。

如果一个 body 没有 joint，它与父 body 固连。

### geom

`geom` 描述几何体，可以使用 box、sphere、capsule、mesh 等类型。它可能参与显示、碰撞和惯量计算，具体取决于属性设置。

### inertial

`inertial` 描述质量、质心和惯量。动力学异常时，质量、惯量和碰撞几何都值得检查。

## 自由基座与 qpos

四足机器人机身通常可以在世界坐标系中自由移动，因此常见：

```xml
<freejoint/>
```

自由基座具有 3 个平移自由度和 3 个旋转自由度。MuJoCo 在 `qpos` 中使用四元数保存姿态，所以自由基座占 7 个 `qpos` 元素；在 `qvel` 中占 6 个元素。

因此：

```text
model.nq
model.nv
```

可能不同。

查看模型维度：

```python
print("nq =", model.nq)
print("nv =", model.nv)
print("nu =", model.nu)

print("qpos =", data.qpos)
print("qvel =", data.qvel)
print("ctrl =", data.ctrl)
```

其中：

| 量 | 含义 |
|---|---|
| `nq` | 广义位置数组长度 |
| `nv` | 广义速度数组长度 |
| `nu` | 控制输入维度 |
| `qpos` | 当前广义位置 |
| `qvel` | 当前广义速度 |
| `ctrl` | 当前执行器控制输入 |

使用这些数组前，需要先确认模型中各关节与索引的对应关系。

## joint 与 actuator

`joint` 描述机械结构允许怎样运动，`actuator` 描述控制输入怎样作用到模型。

例如：

```xml
<actuator>
    <motor name="hip_motor" joint="hip" gear="1"/>
</actuator>
```

这里定义了一个作用于 `hip` 关节的 motor actuator。

对于这种设置，`data.ctrl[i]` 是对应 actuator 的控制输入。实际产生的广义力还会受到 `gear` 等参数影响。

第二次任务要求关节使用力矩模式。零控制量可以写：

```python
data.ctrl[:] = 0.0
```

这表示 actuator 不主动输出控制力矩。机器人仍然受到重力和接触力，因此零力矩状态下依然可能下落、倒下或滑动。

## URDF 到 MJCF

第二次任务要求从 URDF 自行转换得到 MJCF，并在 MJCF 中继续配置场景和执行器。

当前 MuJoCo 本身可以解析 URDF，但 URDF 能表达的内容少于完整 MJCF。任务中仍按原要求完成转换。

建议按下面的顺序处理：

```text
URDF
  ↓
检查 link、joint、mesh
  ↓
转换为 MJCF
  ↓
检查转换结果
  ↓
加入 actuator、场景和初始状态
  ↓
Python 加载
```

转换后先检查：

- mesh 路径；
- body / link 是否完整；
- joint 名称和轴方向；
- joint range；
- base 是否具有自由基座；
- 质量和惯量；
- Viewer 中的姿态是否正常。

模型结构确认以后再开始写控制代码。

## mesh 路径

机器人模型通常会引用 STL 等 mesh 文件。转换以后目录结构可能发生变化，因此需要重新确认相对路径。

MJCF 可以指定 mesh 目录：

```xml
<compiler meshdir="meshes"/>
```

也可以在具体 mesh 中写相对路径：

```xml
<mesh file="..."/>
```

排查路径时可以先检查当前目录和实际文件：

```bash
pwd
find . -maxdepth 3 -type f | head
```

相对路径从主 MJCF 所在位置计算。

## 场景与机器人模型

机器人本体和仿真场景可以分别保存。例如：

```text
robot.xml
scene.xml
```

`robot.xml` 保存机器人结构，`scene.xml` 保存地面、灯光和机器人在世界中的放置方式。

MJCF 可以通过 `<include>` 等方式组合模型。

平面地面可以写：

```xml
<geom name="floor" type="plane" size="5 5 0.1"/>
```

机器人放到地面上时，还需要设置合适的 base 高度和关节初始角度。

## 初始姿态和零力矩

任务要求机器人在平坦地面上静止趴着，并且关节输出力矩为 0。

需要同时处理：

### base 初始位姿

base 太高时会先下落，太低时可能与地面严重穿透。

### 关节初始角

`data.ctrl[:] = 0` 不会改变初始关节角。趴卧姿态需要通过模型默认状态、keyframe 或 Python 中的 `qpos` 设置。

### 接触稳定性

初始几何重叠、惯量异常、摩擦参数不合适都可能导致抖动或滑动。

可以运行一段时间后观察：

```python
print(data.time)
print(data.qpos)
print(data.qvel)
```

如果 `qvel` 长时间保持较大数值，机器人仍在运动。

## 设置初始状态

创建 `MjData` 后可以修改 `qpos`：

```python
model = mujoco.MjModel.from_xml_path("scene.xml")
data = mujoco.MjData(model)

# 根据自己的模型设置初始姿态
# data.qpos[...] = ...

mujoco.mj_forward(model, data)
```

`mj_forward()` 根据当前的 `qpos`、`qvel` 等重新计算依赖状态，但不推进时间。

也可以在 MJCF 中使用 keyframe 保存初始姿态。工程中应明确初始状态到底由 XML 还是 Python 设置，避免同一组状态在多处重复配置。

## 仿真程序骨架

下面只给出仿真循环的结构，模型路径、初始姿态和控制索引需要根据自己的机器狗补充。

```python
import time

import mujoco
import mujoco.viewer

model = mujoco.MjModel.from_xml_path("...")
data = mujoco.MjData(model)

# 根据自己的 qpos 结构设置初始姿态

# 根据 model.nu 和 actuator 定义设置 ctrl

mujoco.mj_forward(model, data)

with mujoco.viewer.launch_passive(model, data) as viewer:
    while viewer.is_running():
        start = time.time()

        mujoco.mj_step(model, data)
        viewer.sync()

        remain = model.opt.timestep - (time.time() - start)
        if remain > 0:
            time.sleep(remain)
```

在自己的模型上需要确认：

- 实际加载哪个 MJCF；
- `qpos` 中 base 和各关节对应哪些索引；
- 趴卧姿态的关节角；
- `model.nu`；
- actuator 和 `ctrl` 的对应关系；
- 如何判断系统已经稳定。

## 仿真步与控制周期

`mj_step(model, data)` 每次推进一个物理时间步。步长由 MJCF 中的 `timestep` 决定：

```xml
<option timestep="0.002"/>
```

这里表示 0.002 s，也就是 500 Hz 的物理步频率。

控制器可以使用更低的频率。例如控制器 50 Hz 更新一次时，可以在两次控制计算之间执行多个 `mj_step()`。

后面阅读强化学习部署代码时，经常会看到这种 control decimation。

## 程序结构

程序变长以后，可以把不同职责拆开：

```text
mujoco_project/
├── models/
│   └── robot.xml
├── scenes/
│   └── flat_scene.xml
├── src/
│   ├── robot.py
│   ├── controller.py
│   └── simulator.py
├── main.py
└── README.md
```

目录不必完全照这个例子安排。拆分时可以按职责考虑：

- 模型文件；
- 场景；
- 状态读取；
- 控制器；
- 仿真循环。

几十行的小程序可以继续放在一个文件中，功能增加后再拆分。

## 阅读 unitree_mujoco

完成基础程序以后，再阅读 `unitree_mujoco`。

可以先定位四类代码。

### 模型加载

搜索：

```text
MjModel
from_xml_path
.xml
scene
```

### 仿真主循环

搜索：

```text
mj_step
while
viewer
```

### 控制输入

搜索：

```text
ctrl
actuator
motor
```

### 状态读取

搜索：

```text
qpos
qvel
sensor
```

先确认：

```text
初始化 → 读取状态 → 计算控制 → mj_step → 显示 / 通信
```

然后再看项目怎样拆分类、线程和通信模块。

### 线程

分析线程时可以记录：

- 哪些任务需要不同运行频率；
- 哪些函数可能阻塞；
- 哪些状态会被多个线程同时访问；
- 是否需要互斥锁或其他同步机制；
- 当前规模下是否可以保持单线程。

线程数量由实际任务决定。

## 常见问题

调试时可以先打印：

```python
print("time:", data.time)
print("nq, nv, nu:", model.nq, model.nv, model.nu)
print("qpos:", data.qpos)
print("qvel:", data.qvel)
print("ctrl:", data.ctrl)
```

数组很长时只查看当前关心的索引范围。

### import 失败

```text
ModuleNotFoundError: No module named 'mujoco'
```

检查：

```bash
which python3
python3 -m pip show mujoco
```

确认安装 MuJoCo 和运行脚本使用的是同一个 Python 环境。

### XML 编译失败

查看错误给出的文件、元素和行号，再检查 XML 结构、属性名称和引用路径。

### mesh 找不到

检查：

- 主 XML 所在目录；
- `meshdir`；
- `<mesh file="..."/>`；
- 实际 mesh 文件位置。

### 仿真开始后弹飞

检查：

- 初始碰撞几何是否重叠；
- base 高度；
- 质量和惯量；
- timestep；
- `ctrl` 是否被误设为很大的值。

### ctrl 写入后关节不动

检查：

- `model.nu` 是否大于 0；
- actuator 是否绑定到目标 joint；
- `ctrl` 索引是否正确；
- `gear` 和控制范围；
- joint 是否达到 limit。

### 零力矩后机器人倒下

零力矩只表示 actuator 不主动输出力矩。机器人在重力作用下仍然会运动。检查初始姿态、base 高度和接触状态。

## C++ 版本

Python 和 C++ 接口写法不同，模型、状态和控制的基本关系保持一致：

```text
model
data
ctrl
step
```

选做 C++ 版本时，可以先把 Python 版本中的模型路径、状态索引和 actuator 对应关系确认清楚，再迁移相同逻辑。

## 完成第二次培训后

至少应当能够说明：

- `MjModel` 与 `MjData` 的区别；
- `qpos`、`qvel`、`ctrl` 的含义；
- 自由基座为什么会使 `nq != nv`；
- joint 与 actuator 的区别；
- motor actuator 的作用；
- 零 `ctrl` 对应什么物理含义；
- 自己怎样从 URDF 得到 MJCF；
- mesh、场景和初始姿态分别在哪里配置；
- 怎样判断机器人是否已经稳定；
- `unitree_mujoco` 中模型加载、状态读取、控制和 stepping 分别位于哪里；
- 引入线程时需要考虑哪些共享状态和同步问题。

## 官方资料

- Modeling / URDF：https://mujoco.readthedocs.io/en/latest/modeling.html
- XML Reference：https://mujoco.readthedocs.io/en/latest/XMLreference.html
- Python：https://mujoco.readthedocs.io/en/latest/python.html
- Programming / Simulation：https://mujoco.readthedocs.io/en/latest/programming/simulation.html
