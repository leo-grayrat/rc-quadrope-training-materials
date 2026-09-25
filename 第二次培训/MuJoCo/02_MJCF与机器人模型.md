# MJCF 与机器人模型

## 6. MJCF 的基本结构

MuJoCo 原生使用 MJCF（MuJoCo Modeling Language）描述模型。MJCF 使用 XML 语法，因此模型通常保存为 `.xml` 文件。一个较完整的 MJCF 文件可能包含大量参数，但目前只需要掌握几个最常用的部分：

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

这些部分承担的作用不同。`compiler` 设置模型解析和编译相关选项，`option` 设置仿真时间步长、重力和求解参数等运行选项，`asset` 保存 mesh、材质和纹理等资源，`worldbody` 描述世界中的刚体结构，`actuator` 描述执行器，`sensor` 描述需要从仿真中读取的传感器量。

本阶段需要优先看懂 `worldbody` 和 `actuator`。四足模型的大部分机械结构都会位于 `worldbody` 中，而真正向关节施加控制作用的部分通常定义在 `actuator` 中。

---

## 7. `body`、`joint` 与 `geom`

MJCF 中的机器人结构按照刚体层级组织。下面是一个简化的单关节结构：

```xml
<worldbody>
    <body name="base" pos="0 0 0.5">
        <geom type="box" size="0.2 0.1 0.05"/>

        <body name="link" pos="0 0 -0.1">
            <joint
                name="joint1"
                type="hinge"
                axis="0 1 0"
            />
            <geom
                type="capsule"
                size="0.03 0.2"
            />
        </body>
    </body>
</worldbody>
```

`body` 表示刚体以及该刚体所使用的局部坐标系。MJCF 通过嵌套的 `body` 建立机器人运动学树，子 `body` 会随着父 `body` 一起运动。四足机器人的机身、大腿、小腿等都可以对应不同的 `body`。

`joint` 定义当前 `body` 相对于父 `body` 允许怎样运动。机器人腿部最常见的是 `hinge`，它表示绕指定轴旋转。常见关节类型还包括 `slide` 和 `free`，分别用于直线运动和六自由度自由运动。

`geom` 描述几何形状，可以参与碰撞，也可以用于显示。一个 `body` 中可以包含多个 `geom`，因此读取模型时不要简单地把 `body` 数量和 `geom` 数量对应起来。实际机器人模型中还经常使用 mesh 几何来表示复杂外形。

对于四足机器人，可以先按照下面的结构理解：

```text
base body
├── front-left hip body
│   └── thigh body
│       └── calf body
├── front-right hip body
│   └── ...
├── rear-left hip body
│   └── ...
└── rear-right hip body
    └── ...
```

各个 `body` 之间通过 `joint` 形成自由度，再由 `geom` 提供碰撞和外观几何。

---

## 8. 自由基座与 `freejoint`

机器人直接放在地面上进行动力学仿真时，机身需要能够整体平移和旋转，因此通常会在基座 `body` 中定义：

```xml
<freejoint/>
```

自由关节提供三维平移和三维旋转共 6 个速度自由度。它在 `qpos` 中使用 7 个数保存基座位姿，其中前三个是位置，后四个表示姿态四元数；在 `qvel` 中使用 6 个数保存线速度和角速度。

因此，对于一个带自由基座并具有 12 个单自由度关节的四足机器人，常见的维度关系是：

```text
qpos: 7 + 12 = 19
qvel: 6 + 12 = 18
```

这也是为什么 `len(data.qpos)` 和 `len(data.qvel)` 往往不同，也不能直接把 `qpos` 的长度理解为关节数量。后续编写强化学习部署代码时，如果需要从 `qpos` 中提取 12 个关节角，就必须先弄清自由基座占用了哪些位置。

可以通过：

```python
print(model.nq)
print(model.nv)
```

确认当前模型的实际维度。面对别人提供的模型时，不要仅根据机器人“有 12 个电机”去猜测 `qpos` 和 `qvel` 的长度。

---

## 9. `inertial`、质量与碰撞

动力学仿真需要知道每个刚体的质量、质心和惯量等参数。MJCF 中可以显式使用 `inertial` 描述这些信息，也可以根据 `geom` 的质量或密度让 MuJoCo 计算相应惯性参数。

例如：

```xml
<body name="link">
    <inertial
        pos="0 0 -0.1"
        mass="1.2"
        diaginertia="0.01 0.02 0.02"
    />
</body>
```

目前不要求推导惯性矩阵，但需要知道质量、质心和惯量会直接影响机器人运动。如果模型在仿真中表现出明显异常，例如轻微接触就快速弹飞、某条腿运动得异常剧烈，除了检查控制器，还应该检查质量、惯量、碰撞几何和关节参数。

机器人外观模型和碰撞模型也可能采用不同的 `geom`。为了让仿真稳定并减少计算量，工程中经常使用较简单的碰撞几何近似复杂外形，例如用 box、capsule 和 sphere 近似腿部和机身。外观 mesh 适合显示，但直接使用非常复杂的 mesh 进行碰撞计算可能增加计算量和调试难度。

---

## 10. URDF 与 MJCF

ROS 和机器人开源项目中经常使用 URDF（Unified Robot Description Format）描述机器人。URDF 中常见的概念包括 `link`、`joint`、`visual`、`collision` 和 `inertial`，它能够很好地描述机器人的基本结构。

MJCF 的组织方式有所不同。URDF 使用若干 `link` 和 `joint` 描述父子关系，MJCF 则通过嵌套的 `body` 直接构成运动学树。两者都能表示机器人结构，但 MJCF 还提供了大量与 MuJoCo 仿真相关的功能，例如默认参数、执行器、传感器和其他仿真配置。

需要特别说明的是，当前版本的 MuJoCo 已经能够解析 URDF，并将其编译为内部模型。因此，从软件能力上看，可以直接让 MuJoCo 读取符合要求的 URDF。培训任务仍然要求大家完成 **URDF → MJCF** 的转换和整理，因为后续需要阅读和修改 MJCF、加入执行器、组织场景，并逐渐接触 MuJoCo 特有的模型参数。显式完成这一过程也有助于理解两种机器人描述方式之间的对应关系。

可以先形成下面的概念：

```text
URDF
link + joint
     ↓
解析 / 转换 / 整理
     ↓
MJCF
body + joint + geom + actuator + ...
     ↓
MuJoCo 编译
     ↓
MjModel
```

转换后的 MJCF 仍然需要检查。尤其要确认 mesh 路径、机器人初始姿态、关节轴、关节范围、质量惯量、碰撞体和执行器等是否符合预期。模型能够成功打开，只能说明 XML 能够被解析，无法自动保证动力学和控制配置全部正确。

---

## 11. 场景与机器人模型

实际项目通常会把机器人和环境分开组织。机器人模型主要描述机器人本体，场景文件再加入地面、灯光、相机和环境物体。这样同一个机器人可以被放入不同场景中使用，而不需要复制大量机器人 XML。

一个简单的工程可以组织为：

```text
mujoco_project/
├── models/
│   └── robot.xml
├── scenes/
│   └── flat_scene.xml
├── scripts/
│   └── simulate.py
└── README.md
```

MJCF 支持通过 `include` 组合多个 XML 文件。例如主场景可以包含其他模型文件：

```xml
<include file="../models/robot.xml"/>
```

`include` 在解析阶段会把被包含文件中的 XML 元素合并到当前模型中，因此最终仍然需要形成一个合法的 MJCF 模型。实际开源项目的文件组织方式可能更加复杂，但阅读时可以先把它们归纳为“机器人本体”“环境场景”“控制程序”三个部分。

平坦地面通常可以用：

```xml
<geom
    name="floor"
    type="plane"
    size="5 5 0.1"
/>
```

表示。对于当前培训任务，先学会加载一个简单平地场景即可。后续进入强化学习训练时，地形会逐渐扩展到坡面、台阶、随机高度场等更复杂形式。

---


## 与培训任务的衔接

这一部分讲到的 URDF、MJCF、mesh 和场景组织，在正式任务中会连续出现。

转换完成以后，可以直接用 Viewer 检查模型：

```bash
python3 -m mujoco.viewer --mjcf=path/to/robot.xml
```

随后再进入 [机器狗任务实现](./04_机器狗任务实现.md)，逐项检查关节、执行器、平地场景和初始姿态。

需要注意 `<include>` 的路径：文件位置按主 MJCF 所在目录解释。MuJoCo 会先把 include 的 XML 合并，再编译整个模型。如果机器人文件和场景文件同时定义 `compiler`、`option` 等全局设置，应检查合并后的最终配置，避免同一参数在两个文件中重复设置。
