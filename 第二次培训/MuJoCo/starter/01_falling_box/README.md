# falling box

对应 [MuJoCo 与 MJCF 模型](../../02_MuJoCo与MJCF模型.md) 和 [入门与最小仿真](../../03_入门与最小仿真.md)。

- `scene.xml`：地面 + 一个带 free joint 的自由方块；
- `simulate.py`：加载该模型并持续调用 `mj_step()`，同时用 Viewer 显示。

运行：

```bash
python3 simulate.py
```

正常情况下方块会在重力作用下落到地面。
