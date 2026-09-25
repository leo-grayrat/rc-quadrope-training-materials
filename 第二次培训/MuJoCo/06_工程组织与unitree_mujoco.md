# 工程组织与 unitree_mujoco

完成基础任务以后，再考虑程序结构。

## 从单文件开始

几十行的 `simulate.py` 可以先放在一个文件中。随着状态读取、控制、日志和通信增加，再逐步拆分：

```text
mujoco_project/
├── models/
├── scenes/
├── src/
│   ├── robot.py
│   ├── controller.py
│   └── simulator.py
└── main.py
```

可以让：

- `simulator.py`：加载模型、持有 `MjData`、推进仿真；
- `robot.py`：整理 joint、actuator 和机器人状态；
- `controller.py`：根据状态生成控制量；
- `main.py`：组合并启动。

这只是一个常见拆法，最终结构应由实际功能决定。

## 阅读 unitree_mujoco

先找到主流程：

```text
README
↓
运行命令
↓
程序入口
↓
模型加载
↓
MjModel / MjData
↓
主循环
↓
状态读取
↓
ctrl 写入
↓
mj_step
```

看到新的类或线程时，先确认它在这条主流程中承担什么职责。

## 线程

基础仿真不需要为了“看起来像工程”强行拆线程。

阅读开源项目时重点回答：

1. 哪个线程推进仿真；
2. 哪个线程产生控制量；
3. Viewer 在哪里更新；
4. 哪些数据在线程之间共享；
5. 为什么这项工作需要独立线程。

仿真、控制、通信、日志具有不同频率或会阻塞时，多线程才开始体现实际作用。

如果多个线程都会接触 `MjData`，还要关注共享状态怎样同步。
