# single joint

这一目录用于 [MuJoCo 与 MJCF 模型](../../02_MuJoCo与MJCF模型.md)、[入门与最小仿真](../../03_入门与最小仿真.md) 和 [状态、执行器与控制](../../04_状态执行器与控制.md)。

- `scene.xml`：固定基座 + 一个 hinge joint，没有 actuator；
- `motor_scene.xml`：在同一模型上增加一个 motor actuator。

学习顺序：

1. 在第 2 篇先看懂 `scene.xml` 的 body / joint / geom 结构；
2. 第 3 篇用 Python 加载它，观察 `qpos`、`qvel` 并修改初始角；
3. 第 4 篇自己加入 actuator，再与 `motor_scene.xml` 对照。
