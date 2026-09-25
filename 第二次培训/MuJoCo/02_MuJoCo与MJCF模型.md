# MuJoCo 与 MJCF 模型

## MuJoCo 在机器人程序里做什么

MuJoCo 是一套刚体动力学和接触仿真引擎。对于四足机器人，它根据刚体质量、惯量、关节结构、碰撞几何和执行器等信息，计算机器人在重力、接触和控制输入作用下怎样运动。

一个最基础的仿真过程可以画成：

```text
机器人模型与场景
        ↓
      MuJoCo
        ↓
位置、速度等当前状态
        ↓
     控制程序
        ↓
力矩或其他控制输入
        ↓
      MuJoCo
        ↓
    下一时刻状态
```

这一篇先只解决第一件事：**MuJoCo 接收到的“机器人模型与场景”到底是什么。** Python 怎样加载模型、状态怎样保存在程序里、怎样推进仿真，都放到下一篇。

## 安装与检查环境

Ubuntu 中先确认 Python 和 pip：

```bash
python3 --version
python3 -m pip --version
```

安装 MuJoCo：

```bash
python3 -m pip install mujoco
```

验证安装：

```bash
python3
```

```python
import mujoco
print(mujoco.__version__)
```

能够输出版本号即可。

以后如果出现“已经安装但 import 失败”，可以检查：

```bash
which python3
python3 -m pip show mujoco
```

确认运行程序的 Python 和安装 MuJoCo 的 Python 是同一个环境。

## MJCF 是什么

MuJoCo 原生使用 MJCF 描述模型。MJCF 文件使用 XML 语法，通常保存为 `.xml`。

XML 最常见的结构是：

```xml
<tag attribute="value">
    ...
</tag>
```

例如：

```xml
<body name="box" pos="0 0 1">
    ...
</body>
```

这里 `body` 是标签名，`name`、`pos` 是属性。

没有内部内容的标签可以写成：

```xml
<freejoint/>
```

标签可以嵌套。MJCF 正是利用这种嵌套关系描述世界和机器人各个刚体之间的结构。

一个常见的 MJCF 顶层结构是：

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

其中：

- `option`：时间步长、重力等仿真参数；
- `asset`：mesh、纹理、材质等资源；
- `worldbody`：世界和机器人刚体结构；
- `actuator`：执行器；
- `sensor`：传感器。

现在先重点看 `worldbody`。执行器等到第四篇再加入。

## 第一个完整模型：falling box

仓库中有：

- [starter/01_falling_box/scene.xml](./starter/01_falling_box/scene.xml)

内容是：

```xml
<mujoco model="falling_box">
    <option timestep="0.002" gravity="0 0 -9.81"/>

    <worldbody>
        <light pos="0 0 3"/>

        <geom
            name="floor"
            type="plane"
            size="5 5 0.1"
            rgba="0.8 0.8 0.8 1"
        />

        <body name="box" pos="0 0 1">
            <freejoint/>
            <geom
                type="box"
                size="0.1 0.1 0.1"
                mass="1"
                rgba="0.2 0.5 0.8 1"
            />
        </body>
    </worldbody>
</mujoco>
```

这份模型可以直接从 XML 读出：

- 世界里有一块 plane 地面；
- 有一个位于 `z=1` 的方块；
- 方块质量为 1；
- 方块带有 `freejoint`，可以在空间中自由平移和旋转；
- 重力为 `0 0 -9.81`；
- 一个物理时间步为 `0.002 s`。

所以还没有写任何控制代码时，就已经能够预测：仿真开始后方块会在重力作用下掉到地面。

## body、joint 和 geom

MJCF 中：

`body` 表示一个刚体以及它自己的局部坐标系。

`joint` 定义这个 body 相对父 body 怎样运动。

`geom` 描述几何形状，可以参与显示、碰撞，或者两者同时参与。

falling box 的方块 body 中有：

```xml
<freejoint/>
```

所以整个方块可以自由运动。

再看一个只有一个旋转关节的模型：

- [starter/02_single_joint/scene.xml](./starter/02_single_joint/scene.xml)

核心结构是：

```xml
<body name="base" pos="0 0 0.6">
    <geom type="box" size="0.10 0.10 0.10" mass="1"/>

    <body name="link" pos="0 0 -0.10">
        <joint
            name="joint1"
            type="hinge"
            axis="0 1 0"
            range="-90 90"
        />
        <geom
            type="capsule"
            fromto="0 0 0 0 0 -0.40"
            size="0.04"
            mass="0.5"
        />
    </body>
</body>
```

它的层级可以画成：

```text
world
└── base
    └── link
        └── joint1
```

`base` 没有 joint，因此固定在 world 上。

`link` 中有一个 `hinge` joint，所以它只能绕 `axis="0 1 0"` 指定的 Y 轴旋转。这个系统只有一个可动自由度。

四足机器人只是把这样的层级扩展得更多。例如：

```text
base
├── front-left hip
│   └── thigh
│       └── calf
├── front-right hip
│   └── ...
├── rear-left hip
│   └── ...
└── rear-right hip
    └── ...
```

## hinge 和 freejoint 需要保存多少状态

一个 hinge joint 只允许绕一个轴转动，因此只需要：

- 一个数表示当前角度；
- 一个数表示当前角速度。

free joint 允许三维平移和三维旋转。

它的位置状态需要 7 个数：

```text
x y z qw qx qy qz
```

前三个是位置，后四个是姿态四元数。

它的速度状态需要 6 个数：

```text
vx vy vz wx wy wz
```

分别表示线速度和角速度。

因此：

```text
一个 hinge:
位置状态 1
速度状态 1

一个 freejoint:
位置状态 7
速度状态 6
```

如果一台四足机器人有一个自由基座和 12 个单自由度腿部关节，那么常见情况就是：

```text
位置状态: 7 + 12 = 19
速度状态: 6 + 12 = 18
```

下一篇会看到 MuJoCo 在 Python 中怎样保存这些状态。

## 质量、惯量和碰撞

动力学仿真还需要每个刚体的质量、质心和惯量。

MJCF 可以显式写：

```xml
<inertial
    pos="0 0 -0.1"
    mass="1.2"
    diaginertia="0.01 0.02 0.02"
/>
```

也可以根据 geom 的质量或密度计算。

外观模型和碰撞模型不一定完全相同。工程中经常用 mesh 显示真实外形，用 box、capsule、sphere 等较简单的 geom 做碰撞近似。

如果模型一接触地面就弹飞，或者某一条腿运动明显异常，除了控制代码，还要检查质量、惯量、碰撞体和 joint 参数。

## URDF 与 MJCF

机器人项目中经常用 URDF 描述机器人。URDF 常见：

- `link`
- `joint`
- `visual`
- `collision`
- `inertial`

MJCF 中有相同或相近的物理概念，但组织方式不同。URDF 用 link 和 joint 表示父子关系，MJCF 主要通过嵌套 body 构成运动学树，并提供 actuator、sensor 等 MuJoCo 仿真配置。

培训任务要求自行完成 URDF → MJCF 转换。转换以后还要检查：

- mesh 路径；
- body 和 joint 层级；
- joint 的 axis 和 range；
- 质量和惯量；
- 碰撞几何；
- 是否存在自由基座。

文件能成功打开，只说明 MuJoCo 能解析它，并不能自动证明动力学配置正确。

## 机器人模型和场景

实际项目通常把机器人本体和环境分开：

```text
mujoco_project/
├── models/
│   └── robot.xml
├── scenes/
│   └── flat_scene.xml
└── simulate.py
```

场景文件可以通过：

```xml
<include file="../models/robot.xml"/>
```

包含机器人模型。

平地可以写成：

```xml
<geom
    name="floor"
    type="plane"
    size="5 5 0.1"
/>
```

到这里，已经知道一个 MJCF 文件大致怎样描述世界、刚体、关节和几何体。下一篇再用 Python 真正加载 falling box 和 single joint，并观察 MuJoCo 怎样把这些模型变成运行状态。
