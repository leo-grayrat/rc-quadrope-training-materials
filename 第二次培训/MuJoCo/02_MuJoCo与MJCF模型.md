# MuJoCo 与 MJCF 模型

## MuJoCo 在机器人程序里做什么

MuJoCo 是一套刚体动力学和接触仿真引擎。对于四足机器人，它根据机器人和环境的结构、质量、关节、碰撞等信息，计算下一时刻会发生什么。

可以先把整个过程理解为：

```text
模型和场景
    ↓
  MuJoCo
    ↓
当前位置、速度等状态
    ↓
控制程序给出新的输入
    ↓
  MuJoCo
    ↓
下一时刻状态
```

这一篇只学习第一层：**模型和场景怎样写出来。** Python 如何加载和运行模型放到下一篇。

## 安装 MuJoCo

Ubuntu 中先检查 Python：

```bash
python3 --version
python3 -m pip --version
```

安装：

```bash
python3 -m pip install mujoco
```

验证：

```bash
python3
```

```python
import mujoco
print(mujoco.__version__)
```

能输出版本号即可。

## 先认识一点 XML

MuJoCo 原生使用 MJCF 描述模型。MJCF 文件使用 XML 语法，通常以 `.xml` 结尾。

先看一个完全空的 MuJoCo 模型：

```xml
<mujoco>
</mujoco>
```

`<mujoco>` 是开始标签，`</mujoco>` 是结束标签。两者之间的内容都属于这个 MuJoCo 模型。

标签可以带属性：

```xml
<mujoco model="example">
</mujoco>
```

这里 `model` 是属性名，`"example"` 是属性值。一个标签可以同时有多个属性：

```xml
<tag a="1" b="2" c="3"/>
```

没有内部内容的标签可以直接用 `/>` 结束。

后面看到：

```xml
<geom type="box" mass="1"/>
```

就可以先读成：

> 这是一个 `geom` 元素，它同时有 `type` 和 `mass` 两个属性。

属性具体代表什么，再由 MJCF 规定。

## 第一步：世界里放一块地面

MuJoCo 把世界中的刚体和几何体放在 `worldbody` 中：

```xml
<mujoco>
    <worldbody>
    </worldbody>
</mujoco>
```

现在往里面加入：

```xml
<geom type="plane" size="5 5 0.1"/>
```

得到：

```xml
<mujoco>
    <worldbody>
        <geom type="plane" size="5 5 0.1"/>
    </worldbody>
</mujoco>
```

`geom` 表示一个几何形状。

这里：

- `type="plane"` 表示它是平面；
- `size="5 5 0.1"` 是这个 geom 的尺寸参数。

不同 `type` 对 `size` 中各个数字的解释并不完全一样。现在只需要知道，这里给场景放了一块足够大的平地；以后需要精确调整具体 geom 时再查 XML Reference。

这个 `geom` 直接放在 `worldbody` 下，因此它属于世界本身，不会像机器人部件一样运动。

## 第二步：世界里再放一个方块

如果只写：

```xml
<geom type="box" size="0.1 0.1 0.1"/>
```

我们只描述了一个 box 形状。

要让它成为一个有自己位置、以后还可以运动的刚体，先建立 `body`：

```xml
<body name="box" pos="0 0 1">
    <geom type="box" size="0.1 0.1 0.1" mass="1"/>
</body>
```

这里新出现了两层东西。

`body` 表示一个刚体节点。它可以有自己的位置、姿态、关节，也可以包含多个 geom。

`geom` 是这个 body 上的几何形状。body 移动时，挂在它下面的 geom 会跟着移动。

```xml
<body name="box" pos="0 0 1">
```

中：

- `name="box"` 给这个 body 一个名字；
- `pos="0 0 1"` 表示它的位置为 x=0、y=0、z=1。

所以方块一开始位于地面上方 1 m。

里面的：

```xml
<geom type="box" size="0.1 0.1 0.1" mass="1"/>
```

表示：

- 几何形状是 box；
- 三个 `size` 值对应 box 在三个方向上的半尺寸，因此这个方块实际边长约为 0.2 m；
- `mass="1"` 表示质量为 1 kg。

现在整个 `worldbody` 可以写成：

```xml
<worldbody>
    <geom type="plane" size="5 5 0.1"/>

    <body name="box" pos="0 0 1">
        <geom type="box" size="0.1 0.1 0.1" mass="1"/>
    </body>
</worldbody>
```

注意此时方块仍然**不能自由运动**。它有 body，并不等于它自动拥有自由度。

## 第三步：让方块能够运动

在方块 body 中加入：

```xml
<freejoint/>
```

变成：

```xml
<body name="box" pos="0 0 1">
    <freejoint/>
    <geom type="box" size="0.1 0.1 0.1" mass="1"/>
</body>
```

`joint` 决定一个 body 相对父级允许怎样运动。

`freejoint` 表示这个 body 可以在三维空间中自由平移和旋转。

现在方块已经具备“能动”的自由度，但还没有给整个仿真设置重力。

## 第四步：给整个仿真设置重力

重力不是某一个 body 自己的属性，而是整个物理世界都要使用的仿真设置，所以它放在 `option` 中：

```xml
<option gravity="0 0 -9.81"/>
```

三个数字分别是 x、y、z 三个方向的重力加速度：

```text
x:  0
y:  0
z: -9.81 m/s²
```

负号表示沿 z 轴负方向。

`option` 还可以同时保存其他全局仿真参数。例如：

```xml
<option timestep="0.002" gravity="0 0 -9.81"/>
```

这里同一个标签有两个属性：

- `gravity`：整个仿真的重力；
- `timestep`：每个物理仿真步对应多少秒。

它们都影响整个仿真怎样运行，因此放在同一个 `option` 中。

到这里，falling box 的核心部分已经全部自己搭出来了：

```text
worldbody
├── 固定的 plane 地面
└── box body
    ├── freejoint
    └── box geom

option
├── gravity
└── timestep
```

仓库里的完整文件在：

- [starter/01_falling_box/scene.xml](./starter/01_falling_box/scene.xml)

现在再打开这个文件时，里面不应该有突然出现的结构。

## body 可以继续嵌套

机器人不是一个完整方块，而是许多刚体连接起来。

先建立一个固定的 base：

```xml
<body name="base" pos="0 0 0.6">
    <geom type="box" size="0.1 0.1 0.1" mass="1"/>
</body>
```

再把另一个 body 放到 base 里面：

```xml
<body name="base" pos="0 0 0.6">
    <geom type="box" size="0.1 0.1 0.1" mass="1"/>

    <body name="link" pos="0 0 -0.1">
        ...
    </body>
</body>
```

这表示 `link` 是 `base` 的子 body。base 如果移动，link 会跟着它一起移动。

## 给子 body 一个旋转关节

在 `link` 中加入：

```xml
<joint name="joint1" type="hinge" axis="0 1 0"/>
```

这里：

- `name="joint1"`：关节名称；
- `type="hinge"`：只能绕一个轴旋转；
- `axis="0 1 0"`：旋转轴是 Y 轴。

如果再加：

```xml
range="-90 90"
```

就表示关节允许的角度范围为 -90° 到 90°。

于是这个小系统可以画成：

```text
world
└── base（固定）
    └── link
        └── joint1（绕 Y 轴旋转）
```

完整的单关节模型放在：

- [starter/02_single_joint/scene.xml](./starter/02_single_joint/scene.xml)

这个文件只是把刚才已经认识的结构组合起来，没有新的核心标签。

## hinge 和 freejoint 会留下多少状态

一个 hinge 只允许绕一个轴转动，因此需要：

- 1 个数表示当前角度；
- 1 个数表示当前角速度。

freejoint 允许三维平移和三维旋转。

它的位置状态需要 7 个数：

```text
x y z qw qx qy qz
```

前三个表示位置，后四个表示姿态四元数。

速度状态需要 6 个数：

```text
vx vy vz wx wy wz
```

所以：

```text
一个 hinge:
位置状态 1
速度状态 1

一个 freejoint:
位置状态 7
速度状态 6
```

如果一台四足机器人有一个自由基座和 12 个 hinge：

```text
位置状态: 7 + 12 = 19
速度状态: 6 + 12 = 18
```

下一篇会看到 MuJoCo 在 Python 中怎样保存这些状态。

## 质量、惯量和碰撞

刚才已经给 geom 写过 `mass="1"`。复杂机器人还需要更完整的质心和惯量信息，例如：

```xml
<inertial
    pos="0 0 -0.1"
    mass="1.2"
    diaginertia="0.01 0.02 0.02"
/>
```

现在只需要知道：

- `mass` 是质量；
- `pos` 可以描述质心相对 body 的位置；
- `diaginertia` 描述三个主轴方向上的转动惯量。

不要求当前推导惯量公式。

实际机器人还常用 mesh 显示外形，用 box、capsule、sphere 等较简单的 geom 参与碰撞。模型出现异常弹飞、接触异常时，除了控制程序，也要检查质量、惯量和碰撞几何。

## URDF 与 MJCF

URDF 中常见：

- `link`
- `joint`
- `visual`
- `collision`
- `inertial`

MJCF 使用嵌套 body 组织机器人，并提供 MuJoCo 自己的仿真配置。

培训任务要求完成 URDF → MJCF 转换。转换以后至少检查：

- body 与 joint 的父子结构；
- joint 名称、轴和范围；
- mesh 路径；
- 质量和惯量；
- 碰撞几何；
- 自由基座是否存在。

文件能打开只说明格式能够被解析，不能自动证明动力学配置正确。

场景与机器人本体也可以分开保存。具体怎样用 `include` 把机器人放进平地场景，等正式机器狗任务时再做；这里先把一个 MJCF 模型本身读懂。
