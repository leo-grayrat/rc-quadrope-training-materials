# MuJoCo 与机器人仿真基础

这一部分面向第二次培训中的 MuJoCo 任务。默认已经完成第一次培训和 C++ 面向对象部分，不要求提前学过 Python、MuJoCo 或机器人动力学。

## 学习顺序

1. [Python 基础](./01_Python基础.md)
2. [入门与最小仿真](./02_入门与最小仿真.md)
3. [MJCF 与机器人模型](./03_MJCF与机器人模型.md)
4. [状态、执行器与控制](./04_状态执行器与控制.md)
5. [机器狗任务实现](./05_机器狗任务实现.md)
6. [工程组织与 unitree_mujoco](./06_工程组织与unitree_mujoco.md)
7. [排错与后续](./07_排错与后续.md)

学习过程使用两个小模型逐步过渡：

```text
falling box
自由刚体、MjModel、MjData、mj_step、Viewer
        ↓
single joint
body、hinge、qpos、qvel、actuator、ctrl
        ↓
完整四足机器人
free base + 12 个关节 + 12 个执行器
```

这样在进入完整机器狗之前，模型结构、状态和控制输入都已经在较小系统中实际运行过。

## 示例

- [starter/01_falling_box](./starter/01_falling_box/)：自由方块，用于第一段完整仿真；
- [starter/02_single_joint](./starter/02_single_joint/)：固定基座加一个旋转关节，用于学习 joint、状态和 actuator。

正式任务要求见 [../任务.md](../任务.md)。人工任务原文保持不变。
