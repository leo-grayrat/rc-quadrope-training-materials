# MJCF 与机器人模型

MuJoCo 原生使用 MJCF 描述模型。MJCF 是 XML 文件。

## 基本结构

常见顶层部分：

- `compiler`：模型编译相关设置；
- `option`：时间步长、重力等仿真参数；
- `asset`：mesh、纹理、材质；
- `worldbody`：世界和机器人刚体；
- `actuator`：执行器；
- `sensor`：传感器。

一个最小结构可以是：

```xml
<mujoco model="example">
    <option timestep="0.002"/>

    <worldbody>
        ...
    </worldbody>
</mujoco>
```

## body、joint、geom

`body` 表示刚体和它的局部坐标系；嵌套的 `body` 构成机器人结构。

`joint` 定义当前 body 相对父 body 怎样运动。四足腿部常见的是 `hinge` 旋转关节。

`geom` 描述几何形状，用于显示、碰撞或两者兼用。

例如：

```xml
<body name="link" pos="0 0 0.5">
    <joint name="joint1" type="hinge" axis="0 1 0"/>
    <geom type="capsule" size="0.03 0.2"/>
</body>
```

这里：

- body 名为 `link`；
- `joint1` 允许它绕 Y 轴转动；
- capsule geom 描述它的几何形状。

## 自由基座

机器狗的机身需要在空间中整体移动，因此基座通常有：

```xml
<freejoint/>
```

free joint 在 `qpos` 中占 7 个数：3 个位置和 4 个四元数；在 `qvel` 中占 6 个速度量。

因此，一个自由基座加 12 个单自由度关节的四足模型通常有：

```text
nq = 7 + 12 = 19
nv = 6 + 12 = 18
```

### 练习：检查 falling box

对 `starter/01_falling_box/scene.xml` 加载模型并输出：

```python
print(model.nq)
print(model.nv)
print(model.nu)
```

方块只有一个 free joint，没有 actuator，因此应得到：

```text
7
6
0
```

## 质量、惯量和碰撞

动力学还需要质量、质心和惯量。MJCF 可以显式定义 `inertial`，也可以从 geom 的质量或密度计算。

外观 mesh 与碰撞形状不必相同。工程中常用 box、capsule、sphere 等简单 geom 做碰撞近似。

模型出现异常弹飞、接触异常或某一部分运动很怪时，除了控制程序，也要检查：

- 质量和惯量；
- 碰撞几何；
- 初始穿模；
- joint 的 axis 和 range。

## URDF 与 MJCF

URDF 常用：

- `link`
- `joint`
- `visual`
- `collision`
- `inertial`

MJCF 用嵌套 body 表示运动学树，并直接提供 MuJoCo 的 actuator、sensor、默认参数等配置。

培训任务要求自行完成 URDF → MJCF 转换。转换完成后仍要检查 mesh 路径、joint、惯量、碰撞体和执行器。

## 机器人与场景分开

可以把机器人和环境分成两个文件：

```text
models/robot.xml
scenes/flat_scene.xml
```

场景中引用机器人：

```xml
<include file="../models/robot.xml"/>
```

平地：

```xml
<geom name="floor" type="plane" size="5 5 0.1"/>
```

### 小练习：建立平地场景

已有 `models/robot.xml`。

要求建立 `scenes/flat_scene.xml`：

- include 机器人；
- 加入一块 plane 地面；
- 用 Viewer 打开场景。

预期结果：

- 机器人和地面同时显示；
- XML 能正常编译；
- mesh 没有缺失。

这里的结果主要通过 Viewer 验证，不要求固定文本输出。
