# single joint

这一目录用于 [MJCF 与机器人模型](../../03_MJCF与机器人模型.md) 和 [状态、执行器与控制](../../04_状态执行器与控制.md)。

- `scene.xml`：固定基座 + 一个 hinge joint，没有 actuator；
- `motor_scene.xml`：在同一模型上增加一个 motor actuator。

直接查看模型：

```bash
python3 -m mujoco.viewer --mjcf=scene.xml
python3 -m mujoco.viewer --mjcf=motor_scene.xml
```

学习顺序是先用 `scene.xml` 理解 body、joint、qpos 和 qvel，再自己尝试加入 actuator；完成后再和 `motor_scene.xml` 对照。
