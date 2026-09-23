# 第二次培训

这一阶段包含 C++ 面向对象 / CMake 前置，以及 MuJoCo 机器人仿真。

## 任务单

本次正式任务要求以原始任务单为准：

- [c++python概念-任务.pdf](./c++python概念-任务.pdf)

其中包括 MuJoCo 安装与了解、URDF 到 MJCF、平坦场景、零力矩、阅读 `unitree_mujoco`，以及选做 C++ 版本等要求。

## 配套讲义

### C++ 面向对象与 CMake

- [机器人队_C++面向对象与CMake入门.md](./机器人队_C++面向对象与CMake入门.md)

讲义从 `Motor` 开始，逐步介绍封装、构造函数、抽象接口、继承与多态、`Robot` 组合、多文件工程与 CMake。关键位置提供起始代码和可验证结果，参考实现放在任务之后。

最后的多文件练习：

- [oop_cmake_starter](./oop_cmake_starter/)

### MuJoCo

- [MuJoCo与机器人仿真基础.md](./MuJoCo与机器人仿真基础.md)

讲义先用小模型解释 `MjModel`、`MjData`、MJCF、`qpos/qvel/ctrl` 和 actuator，再回到任务真正需要处理的机器人模型、平地场景、初始姿态、零力矩和 `unitree_mujoco` 阅读方法。

### 模型资源

- [black](./black/)

包含 URDF 与 mesh 文件。

