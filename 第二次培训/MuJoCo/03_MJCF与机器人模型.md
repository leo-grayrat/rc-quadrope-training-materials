# MJCF 与机器人模型

上一节已经能让 MuJoCo 读取一个 XML 文件。这一节看清楚 XML 里到底描述了什么。

## worldbody、body、joint 和 geom

MJCF 使用 XML。机器人和场景的主要结构放在 `worldbody` 中。

```xml
<worldbody>
    <body name="link" pos="0 0 0.5">
        <joint name="joint1" type="hinge" axis="0 1 0"/>
        <geom type="capsule" size="0.03 0.2"/>
    </body>
</worldbody>
```

这里的 `body` 表示一个刚体以及它自己的坐标系。body 可以继续嵌套 body，因此可以逐层表示机身、大腿、小腿等结构。

`joint` 定义当前 body 相对父 body 怎样运动。四足腿部最常见的是 `hinge`，也就是绕指定轴旋转。

`geom` 描述几何形状。它可以用于显示、碰撞，或者两者同时使用。

一个完整模型还经常出现：

- `compiler`：模型编译相关设置；
- `option`：重力、时间步长等仿真参数；
- `asset`：mesh、纹理和材质；
- `actuator`：执行器；
- `sensor`：传感器。

## 为什么 falling box 的 nq 是 7

falling box 中有：

```xml
<body name="box" pos="0 0 1">
    <freejoint/>
    ...
</body>
```

free joint 允许刚体在三维空间自由平移和旋转。

它在 `qpos` 中用 7 个数保存位姿：

```text
x y z qw qx qy qz
```

前三个是位置，后四个是姿态四元数。

速度状态不需要用四元数表示，因此 free joint 在 `qvel` 中占 6 个数：3 个线速度和 3 个角速度。

所以 falling box 才会得到：

```text
nq = 7
nv = 6
```

同理，一台具有自由基座和 12 个单自由度腿部关节的机器狗，常见情况是：

```text
nq = 7 + 12 = 19
nv = 6 + 12 = 18
```

### 练习：自己解释模型维度

转换完自己的机器狗以后，输出：

```python
print(model.nq)
print(model.nv)
print(model.njnt)
```

要求能够根据 MJCF 中的 free joint 和腿部 joint 数量解释这些数字，而不是只记录输出。

## 质量、惯量和碰撞

MuJoCo 计算动力学还需要每个刚体的质量、质心和惯量。

MJCF 可以显式写：

```xml
<inertial
    pos="0 0 -0.1"
    mass="1.2"
    diaginertia="0.01 0.02 0.02"
/>
```

也可以根据 geom 的质量或密度计算。

显示模型和碰撞模型也不一定相同。实际机器人经常用 mesh 显示外形，用 box、capsule、sphere 等简单 geom 做碰撞近似。

模型出现明显弹飞、接触异常或某一条腿运动很奇怪时，除了控制程序，还要检查质量、惯量、碰撞几何和 joint 参数。

## 从 URDF 到 MJCF

URDF 中常见：

- `link`
- `joint`
- `visual`
- `collision`
- `inertial`

转换成 MJCF 后，机器人结构主要变成嵌套的 body，同时还要补充 MuJoCo 使用的场景和执行器配置。

培训任务要求自行完成 URDF → MJCF。转换成功只说明文件能被解析，仍要检查：

- mesh 是否能找到；
- 四条腿是否完整；
- joint 的 axis 和 range；
- 质量和惯量；
- 碰撞几何；
- 是否存在自由基座。

## 把机器人放进平地场景

机器人本体和环境可以分成两个文件：

```text
models/robot.xml
scenes/flat_scene.xml
```

场景通过：

```xml
<include file="../models/robot.xml"/>
```

包含机器人模型。

平地可以写成：

```xml
<geom name="floor" type="plane" size="5 5 0.1"/>
```

### 练习：建立 flat_scene.xml

给定自己转换好的 `models/robot.xml`，建立 `scenes/flat_scene.xml`。

要求：

- include 机器人模型；
- worldbody 中有一块 plane 地面；
- MuJoCo Viewer 能正常打开该场景。

可以运行：

```bash
python3 -m mujoco.viewer --mjcf=scenes/flat_scene.xml
```

完成后的观察结果应满足：

- 机器人与地面同时出现；
- mesh 没有缺失；
- 机器人结构没有明显错位。

这一练习没有固定文本输出，Viewer 中的模型就是验证结果。
