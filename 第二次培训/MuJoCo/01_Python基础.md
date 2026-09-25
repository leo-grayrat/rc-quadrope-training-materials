# Python 基础

这一部分只补后面 MuJoCo 程序会直接用到的 Python。已经学过 C++ 的类、对象、变量和循环后，很多概念并不陌生，主要需要熟悉 Python 的写法。

## 变量、函数和模块

Python 直接用 `=` 保存一个值：

```python
name = "robot"
count = 3
height = 0.45
```

调用函数时使用圆括号：

```python
print(name)
print(count)
```

Python 的功能可以来自不同模块。导入模块：

```python
import math

print(math.sqrt(9))
```

这里的 `math.sqrt` 表示模块 `math` 中的 `sqrt` 函数。后面看到 `mujoco.mj_step`、`mujoco.MjData` 时，点号也是同样的访问方式。

## 对象和成员

C++ 中可以写：

```cpp
motor.getPosition();
```

Python 中也使用点号访问对象中的数据和函数：

```python
motor.position
motor.get_position()
```

因此后面的：

```python
model.nq
data.qpos
viewer.sync()
```

都可以先按“对象.成员”或“对象.函数”来读。具体成员表示什么，由 MuJoCo 再规定。

## 列表、下标和切片

Python 的下标从 0 开始：

```python
numbers = [10, 20, 30, 40]

print(numbers[0])
print(numbers[2])
```

输出：

```text
10
30
```

还可以一次取出一段：

```python
print(numbers[1:3])
print(numbers[2:])
print(numbers[:])
```

其中：

- `[1:3]` 取下标 1、2；
- `[2:]` 从下标 2 取到最后；
- `[:]` 表示全部元素。

后面 MuJoCo 的 `qpos`、`qvel`、`ctrl` 都会用到类似写法。

## 判断和循环

Python 用缩进表示代码块：

```python
value = 3

if value > 0:
    print("positive")
```

固定重复若干次：

```python
for i in range(3):
    print(i)
```

输出：

```text
0
1
2
```

持续循环：

```python
running = True

while running:
    print("running")
    running = False
```

MuJoCo 的仿真循环本质上也是一个 `while`：窗口还开着，就继续推进下一步。

## with

后面 Viewer 示例会出现：

```python
with something() as viewer:
    ...
```

这里先理解为：创建一个对象，把它在这段代码里命名为 `viewer`；离开代码块时，相关资源会被自动清理。当前阶段不需要深入 Python 的上下文管理器机制。

## 一个很小的练习

写出程序，使它依次输出列表中的三个关节名：

```python
joints = ["hip", "thigh", "calf"]
```

预期输出：

```text
hip
thigh
calf
```

这一步只需要用到变量、列表和 `for`。后面遍历机器人关节时会使用同样的结构。
