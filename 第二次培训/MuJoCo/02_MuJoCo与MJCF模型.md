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

可以读成：这是一个几何体元素 `geom`，它同时有 `type` 和 `mass` 两个属性。属性具体代表什么，再由 MJCF 规定。

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

`geom` 表示几何形状。这里：

- `type="plane"` 表示平面；
- `size="5 5 0.1"` 是这个几何体的尺寸参数。

不同 `type` 对 `size` 中各个数字的解释并不完全一样。现在只需要知道，这里给场景放了一块足够大的平地；以后需要精确调整具体几何体时再查 XML Reference。

这个 `geom` 直接放在 `worldbody` 下，因此它属于世界本身，不会像机器人部件一样运动。

## 第二步：世界里再放一个方块

如果只写：

```xml
<geom type="box" size="0.1 0.1 0.1"/>
```

我们只描述了一个长方体形状。

要让它成为一个有自己位置、以后还可以运动的刚体，先建立刚体节点 `body`：

```xml
<body name="box" pos="0 0 1">
    <geom type="box" size="0.1 0.1 0.1" mass="1"/>
</body>
```

这里新出现了两层东西。

`body` 表示一个刚体节点。它可以有自己的位置、姿态、关节，也可以包含多个几何体。

`geom` 是这个刚体上的几何形状。刚体移动时，挂在它下面的几何体会跟着移动。

```xml
<body name="box" pos="0 0 1">
```

中：

- `name="box"` 给这个刚体一个名字；
- `pos="0 0 1"` 表示它的位置为 x=0、y=0、z=1。

所以方块一开始位于地面上方 1 m。

里面的：

```xml
<geom type="box" size="0.1 0.1 0.1" mass="1"/>
```

表示：

- 几何形状是长方体；
- 三个 `size` 值对应三个方向上的半尺寸，因此这个方块实际边长约为 0.2 m；
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

注意此时方块仍然 **不能自由运动**。模型里有一个刚体节点，并不等于它自动拥有自由度；刚体相对父级能怎样运动，要由关节决定。

## 第三步：让方块能够运动

在方块刚体中加入自由关节：

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

关节决定一个刚体相对父级允许怎样运动。`<freejoint/>` 表示自由关节，它允许这个刚体在三维空间中自由平移和旋转。

现在方块已经具备运动自由度，但还没有给整个仿真设置重力。

## 第四步：给整个仿真设置重力

重力是整个物理世界都要使用的仿真设置，所以它放在 `option` 中：

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

到这里，下落方块示例的核心结构已经全部搭出来了：

```text
世界 worldbody
├── 固定地面 geom
└── 方块刚体 body
    ├── 自由关节 <freejoint/>
    └── 方块几何体 geom

全局设置 option
├── gravity
└── timestep
```

仓库里的完整文件在：

- [starter/01_falling_box/scene.xml](./starter/01_falling_box/scene.xml)

`01_falling_box` 只是目录名；正文后面统一称它为“下落方块示例”。

## 刚体可以继续嵌套

机器人由许多刚体连接起来。

先建立一个固定基座：

```xml
<body name="base" pos="0 0 0.6">
    <geom type="box" size="0.1 0.1 0.1" mass="1"/>
</body>
```

再把另一个刚体放到基座里面：

```xml
<body name="base" pos="0 0 0.6">
    <geom type="box" size="0.1 0.1 0.1" mass="1"/>

    <body name="link" pos="0 0 -0.1">
        ...
    </body>
</body>
```

这表示 `link` 是 `base` 的子刚体。基座如果移动，子刚体会跟着它一起移动。

## 给子刚体一个单轴转动关节

在 `link` 中加入：

```xml
<joint name="joint1" type="hinge" axis="0 1 0"/>
```

这里：

- `name="joint1"`：关节名称；
- `type="hinge"`：这是一个单轴转动关节，只能绕一个轴旋转；
- `axis="0 1 0"`：旋转轴是 Y 轴。

后文在自然语言里称它为“单轴转动关节”；`hinge` 只保留为 MJCF 中的 `type` 取值。

如果再加：

```xml
range="-90 90"
```

就表示关节允许的角度范围为 -90° 到 90°。

给 `link` 自己也加一个长方体外形：

```xml
<geom
    type="box"
    pos="0 0 -0.2"
    size="0.04 0.04 0.2"
    mass="0.5"
/>
```

这里没有新的结构：

- `geom` 仍然是几何形状；
- `type="box"` 表示长方体；
- `size` 是三个方向上的半尺寸；
- `mass` 是质量；
- `pos="0 0 -0.2"` 表示这个几何体相对所属的 `link` 刚体再向 z 负方向偏移 0.2 m。

单关节示例中还写了：

```xml
<option gravity="0 0 0"/>
```

这里故意把重力设为 0，是为了后面学习关节和执行器时，杆不会在还没施加控制之前自己掉下来。它和下落方块中的 `option` 是同一个概念。

于是这个小系统可以画成：

```text
世界
└── 固定基座 base
    └── 子连杆 link
        └── joint1：单轴转动关节，绕 Y 轴旋转
```

完整模型放在：

- [starter/02_single_joint/scene.xml](./starter/02_single_joint/scene.xml)

`02_single_joint` 同样只是目录名；正文后面统一称它为“单关节模型”。

## 关节怎样决定状态量的数量

`qpos` 和 `qvel` 保存的是模型中 **允许发生的运动** 对应的状态。固定的刚体本身不会因为“存在一个 `body`”就额外占一组状态；真正决定状态量数量的是关节给系统增加了哪些运动自由度。

单轴转动关节只允许绕一个轴转动，因此需要：

- 1 个数表示当前关节角；
- 1 个数表示当前关节角速度。

自由关节允许三维平移和三维旋转。它的位置和姿态需要 7 个数：

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
一个单轴转动关节（type="hinge"）：
位置状态 1
速度状态 1

一个自由关节（<freejoint/>）：
位置/姿态状态 7
速度状态 6
```

这也解释了两个示例中 `qpos` 和 `qvel` 为什么需要保存不同数量的状态量：

```text
下落方块：
只有一个自由关节
→ qpos 有 7 个元素，qvel 有 6 个元素

单关节模型：
基座固定，不提供自由度
只有 joint1 这个单轴转动关节
→ qpos 有 1 个元素，qvel 有 1 个元素
```

如果一台四足机器人有一个自由基座和 12 个单轴转动关节：

```text
位置状态: 7 + 12 = 19
速度状态: 6 + 12 = 18
```

下一篇会看到 MuJoCo 在 Python 中怎样把这些状态保存到 `qpos` 和 `qvel`。

## 质量、惯量和碰撞

刚才已经给几何体写过 `mass="1"`。复杂机器人还需要更完整的质心和惯量信息，例如：

```xml
<inertial
    pos="0 0 -0.1"
    mass="1.2"
    diaginertia="0.01 0.02 0.02"
/>
```

现在只需要知道：

- `mass` 是质量；
- `pos` 可以描述质心相对刚体的位置；
- `diaginertia` 描述三个主轴方向上的转动惯量。

不要求当前推导惯量公式。

实际机器人还常用网格模型（`mesh`）显示外形，用长方体、胶囊体、球体等较简单的几何体参与碰撞。模型出现异常弹飞、接触异常时，除了控制程序，也要检查质量、惯量和碰撞几何。

## URDF 与 MJCF

URDF 中常见的元素包括 `link`、`joint`、`visual`、`collision`、`inertial`。这些是文件格式中的实际名称，阅读 URDF 时仍然需要认识。

MJCF 使用嵌套的刚体节点 `body` 组织机器人，并提供 MuJoCo 自己的仿真配置。

培训任务要求完成 URDF → MJCF 转换。转换以后至少检查：

- 刚体与关节的父子结构；
- 关节名称、轴和范围；
- 网格文件路径；
- 质量和惯量；
- 碰撞几何；
- 自由基座是否存在。

文件能打开只说明格式能够被解析，不能自动证明动力学配置正确。

场景与机器人本体也可以分开保存。具体怎样用 `include` 把机器人放进平地场景，等正式机器狗任务时再做；这里先把一个 MJCF 模型本身读懂。
