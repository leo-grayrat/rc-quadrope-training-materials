# Starter 代码

- `01_motor/`：类、对象、成员变量与成员函数
- `02_encapsulation/`：封装、构造函数、`const`
- `03_polymorphism/`：抽象接口、继承与多态
- `04_robot/`：组合与 `Motor&`
- `05_cmake/`：多文件工程与 CMake

各目录中的 TODO 对应正文中的同编号练习。

## 自测

每个练习目录都提供 `test.sh`。完成 TODO 后进入对应目录运行：

```bash
bash test.sh
```

测试全部放在仓库中，可以直接查看。它们用于补充正文中的示例运行：

- `01_motor`：额外对象和输出格式；
- `02_encapsulation`：上下边界、非法输入和状态保持；
- `03_polymorphism`：继承关系和通过 `Motor&` 的调用；
- `04_robot`：使用测试电机检查两侧调用次数和状态；
- `05_cmake`：CMake 配置、构建、demo 输出和类行为。

`test.sh` 返回 0 表示全部通过；任一检查失败时返回非 0。
