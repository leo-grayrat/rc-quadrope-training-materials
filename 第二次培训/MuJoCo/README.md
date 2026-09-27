# MuJoCo 与机器人仿真基础

这一部分面向第二次培训中的 MuJoCo 任务。默认已经完成第一次培训和 C++ 面向对象部分，不要求提前学过 Python、MuJoCo 或机器人动力学。

## 学习顺序

1. [Python 基础](./01_Python基础.md)
2. [MuJoCo 与 MJCF 模型](./02_MuJoCo与MJCF模型.md)
3. [入门与最小仿真](./03_入门与最小仿真.md)
4. [状态、执行器与控制](./04_状态执行器与控制.md)
5. [机器狗任务实现](./05_机器狗任务实现.md)
6. [工程组织与 unitree_mujoco](./06_工程组织与unitree_mujoco.md)
7. [排错与后续](./07_排错与后续.md)
8. [从 MuJoCo joint 到真实关节电机](./08_从MuJoCo关节到真实关节电机.md)

前七篇完成第二次培训本身；第八篇放在第二次和第三次之间，建立 MuJoCo 中的 `qpos / qvel / ctrl` 与真实关节中的角度、角速度、力矩、转子、减速器、编码器和电机接口之间的对应关系。

学习过程使用两个小模型逐步过渡：

```text
falling box / single joint
先看懂 MJCF、body、joint、geom、freejoint
        ↓
Python 加载与运行
MjModel、MjData、qpos、qvel、mj_step、Viewer
        ↓
single joint + motor
actuator、ctrl
        ↓
完整四足机器人
free base + 12 个关节 + 12 个执行器
        ↓
真实关节驱动链
关节控制 → 减速器 → 转子 → 编码器 → SDK
```

这样在进入完整机器狗之前，模型结构、状态和控制输入都已经在较小系统中实际运行过；进入第三次培训以前，再把仿真里被压缩掉的机械和接口层展开。

## 示例

- [starter/01_falling_box](./starter/01_falling_box/)：自由方块，用于第一段完整仿真；
- [starter/02_single_joint](./starter/02_single_joint/)：固定基座加一个旋转关节，用于学习 joint、状态和 actuator。

正式任务要求见 [../任务.md](../任务.md)。人工任务原文保持不变。
