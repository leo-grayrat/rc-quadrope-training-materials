# quadruped starter

这里不提供机器狗 MJCF，也不提供关节初始角。

先把自己转换和整理后的模型放到合适目录，然后：

```bash
python3 model_info.py path/to/robot.xml
```

确认：

- `nq`、`nv`、`nu`；
- 自由基座；
- 各关节名称与 `qpos` 地址；
- actuator 数量；
- actuator 对应的关节。

随后在 `simulate.py` 中填写：

- `INITIAL_BASE_HEIGHT`；
- `INITIAL_JOINT_POSITIONS`。

运行：

```bash
python3 simulate.py path/to/scene.xml
```

程序会保持 `data.ctrl[:] = 0.0`。是否能够稳定趴下取决于模型、初始姿态和接触状态。
