# Python 基础

这一部分只介绍后面 MuJoCo 代码会直接用到的 Python 语法。

## 导入模块

Python 用 `import` 导入其他模块：

```python
import mujoco
import time
```

导入后可以用点号访问模块中的内容：

```python
mujoco.MjModel
time.sleep
```

## 变量、对象和点号

```python
model = mujoco.MjModel.from_xml_path("scene.xml")
```

`=` 把右边得到的对象保存到变量 `model`。

Python 中经常用：

```text
对象.成员
对象.函数(...)
```

访问对象中的数据或函数。例如：

```python
model.nq
data.qpos
viewer.sync()
```

前面的 C++ 培训中已经接触过类和对象。Python 的写法不同，但 `model`、`data` 同样可以理解为某个类创建出的对象。

## 函数调用

圆括号表示调用函数：

```python
print(model.nq)
mujoco.mj_step(model, data)
```

括号中的内容是传给函数的参数。多个参数用逗号分开。

## 判断和循环

Python 用缩进表示代码块：

```python
if model.nu > 0:
    data.ctrl[:] = 0.0
```

```python
for i in range(10):
    mujoco.mj_step(model, data)
```

```python
while viewer.is_running():
    mujoco.mj_step(model, data)
```

上面分别表示条件判断、重复 10 次和持续循环。

## 下标和切片

```python
data.qpos[0]
data.qpos[0:3]
data.qpos[7:]
data.ctrl[:]
```

含义分别是：

- `[0]`：第 1 个元素；
- `[0:3]`：下标 0、1、2；
- `[7:]`：从下标 7 到最后；
- `[:]`：全部元素。

因此：

```python
data.ctrl[:] = 0.0
```

表示把 `ctrl` 中所有元素都设为 `0.0`。

## with

Viewer 示例中会出现：

```python
with mujoco.viewer.launch_passive(model, data) as viewer:
    ...
```

这里可以先理解为：创建 Viewer，并在这个代码块中把它叫作 `viewer`。离开代码块时，相关资源会被自动清理。

后面的讲义默认已经认识这些写法。遇到新的 Python 语法时再补充。
