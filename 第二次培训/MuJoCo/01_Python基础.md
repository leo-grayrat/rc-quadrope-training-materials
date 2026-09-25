# Python 基础

这一部分只介绍后面 MuJoCo 程序会直接用到的 Python 语法。

## 模块、变量和对象

Python 用 `import` 导入模块：

```python
import mujoco
import time
```

点号表示访问模块、类或对象中的成员：

```python
mujoco.MjModel
model.nq
data.qpos
viewer.sync()
```

前面的 C++ 培训已经介绍过类和对象。这里可以沿用同样的理解：一个对象里面保存数据，也可以提供函数。Python 中通常通过 `对象.成员` 和 `对象.函数(...)` 使用它们。

```python
model = mujoco.MjModel.from_xml_path("scene.xml")
```

`=` 把右边得到的对象保存到变量 `model`。圆括号表示调用函数，括号里是传入的参数。

## 判断和循环

Python 用缩进表示代码块：

```python
if model.nu > 0:
    data.ctrl[:] = 0.0
```

重复固定次数：

```python
for i in range(10):
    mujoco.mj_step(model, data)
```

持续循环：

```python
while viewer.is_running():
    mujoco.mj_step(model, data)
```

## 下标和切片

```python
data.qpos[0]
data.qpos[0:3]
data.qpos[7:]
data.ctrl[:]
```

- `[0]`：第 1 个元素；
- `[0:3]`：下标 0、1、2；
- `[7:]`：从下标 7 到最后；
- `[:]`：全部元素。

因此：

```python
data.ctrl[:] = 0.0
```

表示把 `ctrl` 中所有元素设为 `0.0`。

## with

Viewer 示例中会出现：

```python
with mujoco.viewer.launch_passive(model, data) as viewer:
    ...
```

这里先理解为：创建 Viewer，并在这个代码块中把它保存为 `viewer`。离开代码块时，相关资源会被自动清理。

后面遇到新的 Python 语法时再补充，不需要先学完整的 Python 语言。
