# MJCF 与机器人模型

上一节已经运行了 falling box。现在开始看 MJCF 本身是怎样描述一个机械系统的。

## 先认识 XML 的基本结构

MJCF 使用 XML。最常见的形式是：

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

表示一个名为 `box`、位置为 `0 0 1` 的 body。

没有内部内容的标签可以写成：

```xml
<freejoint/>
```

标签可以嵌套。子标签属于父标签描述的结构。MJCF 正是利用这种嵌套关系表示机器人各个刚体之间的连接。

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

当前最重要的是 `worldbody` 和 `actuator`。前者描述世界和机器人的刚体结构，后者描述怎样向关节施加控制。

## body、joint 和 geom

MJCF 中，`body` 表示一个刚体以及它自己的局部坐标系。

`joint` 定义这个 body 相对于父 body 怎样运动。

`geom` 描述几何形状，可以参与显示、碰撞或两者同时参与。

看一个只有一个旋转关节的模型：

```xml
<mujoco model="single_joint">
    <option timestep="0.002" gravity="0 0 0"/>

    <worldbody>
        <light pos="0 0 3"/>

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
    </worldbody>
</mujoco>
```

这里的结构是：

```text
world
└── base
    └── link
        └── joint1
```

`base` 没有 joint，因此固定在 world 上。

`link` 中有一个 `hinge` joint，所以它可以绕 `axis="0 1 0"` 指定的 Y 轴旋转。这个系统只有一个可动自由度。

仓库中提供了这份模型：

- [starter/02_single_joint/scene.xml](./starter/02_single_joint/scene.xml)

直接用 Viewer 打开：

```bash
python3 -m mujoco.viewer --mjcf=starter/02_single_joint/scene.xml
```

因为当前重力为 0，又没有执行器，杆不会自己运动。

## 一个 joint 怎样进入 qpos 和 qvel

这个模型只有一个 hinge joint。

加载后：

```python
model = mujoco.MjModel.from_xml_path(
    "starter/02_single_joint/scene.xml"
)
data = mujoco.MjData(model)

print(model.nq)
print(model.nv)
print(data.qpos)
print(data.qvel)
```

应得到类似：

```text
1
1
[0.]
[0.]
```

这里非常直观：

- 一个 hinge joint 需要一个数表示当前角度，所以 `nq = 1`；
- 它也需要一个数表示当前角速度，所以 `nv = 1`；
- `data.qpos[0]` 就是当前关节角；
- `data.qvel[0]` 就是当前关节角速度。

这就是后面四足机器狗中 `qpos`、`qvel` 的最小版本。

### 练习：改变初始关节角

加载 single joint 模型以后：

1. 打印初始 `data.qpos`；
2. 把 `data.qpos[0]` 改成 `0.5`；
3. 调用 `mujoco.mj_forward(model, data)`；
4. 再打印 `data.qpos`；
5. 打开 Viewer 观察杆的姿态变化。

这里 `0.5` 的单位是弧度。

`mj_forward()` 会根据你刚刚修改的状态重新计算运动学等相关量，但不会推进仿真时间。因此这个练习适合观察“状态改变以后，模型姿态怎样跟着改变”。

## freejoint 为什么不同

falling box 中使用的不是 hinge，而是：

```xml
<freejoint/>
```

free joint 允许刚体在三维空间中自由平移和旋转。

它在 `qpos` 中占 7 个数：

```text
x y z qw qx qy qz
```

前三个表示位置，后四个是姿态四元数。

它在 `qvel` 中占 6 个数：

```text
vx vy vz wx wy wz
```

分别表示线速度和角速度。

因此 falling box 才会出现：

```text
nq = 7
nv = 6
```

四足机器人通常可以看成：

```text
一个自由基座
+
若干个 hinge joint
```

如果机器人有 12 个单自由度腿部关节，那么常见情况就是：

```text
qpos: 7 + 12 = 19
qvel: 6 + 12 = 18
```

现在这个关系已经不是需要死记的数字，而是由模型结构直接得到。

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

外观模型和碰撞模型也不一定完全相同。工程中常用较简单的 box、capsule、sphere 做碰撞近似，再用 mesh 显示真实外形。这样通常比直接用复杂 mesh 做碰撞更稳定，也更容易调试。

因此如果模型一接触地面就弹飞，或者某一条腿运动明显异常，需要同时检查控制、质量惯量、碰撞体和关节参数。

## URDF 与 MJCF

机器人项目中经常用 URDF 描述机器人。URDF 常见：

- `link`
- `joint`
- `visual`
- `collision`
- `inertial`

MJCF 中也有这些物理概念，但组织方式不同。URDF 用多个 link 和 joint 描述父子关系，MJCF 主要通过嵌套 body 形成运动学树，并且直接提供 actuator、sensor 和 MuJoCo 仿真配置。

培训任务要求自行完成 URDF → MJCF 转换。转换以后要继续检查：

- mesh 路径；
- body 和 joint 层级；
- joint 的 axis 和 range；
- 质量和惯量；
- 碰撞几何；
- 是否存在自由基座。

文件能成功打开，只能说明 MuJoCo 能解析它，不能自动证明动力学配置全部正确。

## 机器人模型和场景

实际项目常把机器人本体和环境分开：

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

下一篇先在 single joint 上加入执行器和控制输入，再把同样的机制扩展到完整四足模型。
