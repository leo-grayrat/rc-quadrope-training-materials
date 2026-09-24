# Linux 系统与基础操作

这份讲义配合第一次培训的 [任务.md](./任务.md) 使用。任务文件保留原始要求，本文不改写任务，也不直接给出任务的完整操作答案。

这一阶段真正需要建立的是一套基本的 Linux 操作直觉：知道自己现在在哪个目录、一个路径指向什么、为什么某个文件“存在但不能直接运行”、Shell 到底怎样找到一个命令，以及改过系统配置以后怎样恢复。后续 ROS 2、MuJoCo、强化学习环境和实机部署都会反复遇到这些问题。

---

## 1. 终端里到底发生了什么

Ubuntu 中按 `Ctrl + Alt + T` 可以打开 Terminal。Terminal 只是显示输入和输出的窗口，真正解释命令的是 Shell；Ubuntu 默认常见的是 Bash。

例如：

```bash
python3 hello.py
```

Bash 会先找到 `python3`，再把 `hello.py` 作为参数交给 Python。

而：

```bash
./hello.py
```

含义完全不同：这次不是“让 Python 打开文件”，而是“把 `hello.py` 本身当作一个可执行程序运行”。这两个动作看起来只差几个字符，但后面关于权限、shebang、PATH 的很多问题都来自这个区别。

遇到命令不理解时，可以先看：

```bash
command --help
man command
```

---

## 2. 路径：先回答“我现在在哪里”

Linux 文件系统从根目录 `/` 开始。普通用户自己的文件通常放在 `/home/用户名`，Bash 中可以用 `~` 表示当前用户的 home。

几个最基础的命令：

```bash
pwd
ls
cd 目录
cd ..
mkdir 目录
touch 文件
```

其中 `.` 表示当前目录，`..` 表示父目录。

### 绝对路径与相对路径

`/home/leo/robocon/learn` 是绝对路径，因为它从 `/` 开始；如果当前已经位于 `/home/leo/robocon`，那么 `learn` 和 `./learn` 都是相对当前目录解释的路径。

`~/robocon/learn` 也能唯一定位文件，但 `~` 是 Shell 展开的 home 简写。

做培训任务时，建议经常执行：

```bash
pwd
ls -l
```

很多“文件找不到”其实只是当前目录与自己想象的不一致。

---

## 3. 创建、复制、移动与删除

常用操作：

```bash
mkdir -p a/b/c
cp source.txt copy.txt
mv old.txt new.txt
rm file.txt
```

`mv` 同时承担“移动”和“重命名”。查看文本文件可以使用 `cat`、`less`、`head`、`tail`。

删除操作尤其要慢一点。培训初期不要养成看到问题就直接 `rm -rf ...` 的习惯；`rm` 通常没有类似桌面回收站的恢复流程。

---

## 4. 为什么 Python 文件有时能运行、有时不能

先建立一个和正式任务无关的小例子：

```python
print("hello")
```

假设它保存为 `demo.py`。

### 交给解释器运行

```bash
python3 demo.py
```

这里要求的是 Bash 能找到 `python3`，并且 Python 能读取 `demo.py`。`demo.py` 自己不需要具有执行权限。

### 直接执行文件

如果写：

```bash
./demo.py
```

系统会把文件本身当成程序执行。此时至少有两个新问题。

第一，文件是否有执行权限：

```bash
ls -l demo.py
chmod u+x demo.py
```

第二，系统必须知道“用什么解释这个文本文件”。脚本第一行通常使用 shebang：

```python
#!/usr/bin/env python3

print("hello")
```

因此，**能被 `python3 file.py` 运行，不等于这个文件本身已经是一个可以直接执行的命令。**

这正是第一次任务中要求观察失败、再解释原因的核心知识之一。做任务时请自己完成对应操作，不要把这里的小例子机械替换文件名当作答案。

---

## 5. 为什么 `./demo.py` 能运行，输入 `demo.py` 却可能不行

Bash 收到 `demo.py` 时，不会默认搜索当前目录，而会按环境变量 `PATH` 中列出的目录依次寻找可执行文件。

查看 PATH：

```bash
echo "$PATH"
```

查看一个命令实际来自哪里：

```bash
which python3
command -v python3
```

假设你有一个专门存放自己脚本的目录 `/home/用户名/bin`，可以在当前 Shell 中临时加入：

```bash
export PATH="$HOME/bin:$PATH"
```

关闭终端后，这个临时修改不会自动保留。要让新打开的 Bash 也使用这项设置，需要把对应命令放进 `~/.bashrc`。

修改配置前建议先备份：

```bash
cp ~/.bashrc ~/.bashrc.backup
```

修改后可以：

```bash
source ~/.bashrc
```

在当前终端重新加载。任务最后要求恢复修改，所以实验时必须知道自己改了哪几行，而不是只知道“复制一条命令进去”。

---

## 6. `.bashrc` 为什么能影响新终端

Bash 启动交互式 Shell 时会读取用户的 `~/.bashrc`。因此如果在其中加入一条输出命令，每次新开终端都会再次执行。

这同时说明为什么不应该随意把大量启动命令塞进 `.bashrc`：如果写错，每个新终端都会重复出问题。

排查最近修改可以用：

```bash
tail -n 20 ~/.bashrc
```

正式任务要求最后恢复相关修改。恢复时优先删除自己增加的行；如果使用备份，也要确认备份确实来自修改之前。

---

## 7. Shell 脚本与执行权限

Bash 脚本最小形式：

```bash
#!/usr/bin/env bash

echo "hello"
```

保存为 `demo.sh` 后，可以：

```bash
bash demo.sh
```

也可以在具有执行权限时直接：

```bash
./demo.sh
```

第二种方式仍然依赖执行权限和 shebang。Python 脚本与 Bash 脚本在“直接执行”这件事上的原理是相通的。任务让你把同一流程再用 Bash 做一次，重点就是确认这个规律，而不是记住两套无关命令。

---

## 8. 软件安装、deb 与 CPU 架构

Ubuntu 常用 `apt` 管理软件包：

```bash
sudo apt update
sudo apt install 包名
```

下载到本地的 `.deb` 可以使用：

```bash
sudo apt install ./package.deb
```

这里的 `./` 告诉 apt 这是当前目录中的本地文件。

查看机器架构：

```bash
uname -m
dpkg --print-architecture
```

常见对应关系：

| 常见名称 | 含义 |
|---|---|
| `x86_64` / `amd64` | 常见 Intel / AMD 64 位电脑 |
| `aarch64` / `arm64` | ARM 64 位平台 |

下载 deb 时必须匹配架构，也应只使用可信来源。

---

## 9. 通配符、正则表达式与 `/dev`

第一次任务会让你查看 `/dev` 中包含 `tty` 的设备文件。先区分两种匹配。

Shell 通配符是在命令执行前由 Bash 展开：

```bash
ls /dev/tty*
```

而：

```bash
ls /dev | grep 'tty'
```

是先产生文本，再由 `grep` 过滤。二者不是同一种语法。

`/dev` 也不是普通资料目录。Linux 会把很多设备暴露为特殊文件。以后机器人常见 `/dev/ttyUSB0`、`/dev/ttyACM0`，它们可能对应 USB 转串口等设备。

如果程序能看到设备却打不开，除了代码错误，还要检查：

```bash
ls -l /dev/ttyUSB0
```

确认设备权限和所属用户组。

---

## 10. 进程：程序运行以后去了哪里

前台运行程序时，终端会等待它结束。查看进程可以使用：

```bash
ps
ps aux
```

查找某类进程：

```bash
ps aux | grep python
```

`Ctrl + C` 通常向当前前台程序发送中断信号。

第一次任务不要求深入进程管理，但之后跑仿真、ROS 2 节点和训练程序时，至少要有“程序是一个进程，而不是终端窗口本身”的概念。

---

## 11. 做任务时的排错顺序

如果某一步失败，不要随机改命令，先判断是哪一层。

**文件在哪里**

```bash
pwd
ls -l
```

**文件内容对不对**

```bash
cat 文件名
head 文件名
```

**自己现在是哪种运行方式**

想清楚下面三条为什么不同：

```text
python3 file.py
./file.py
file.py
```

直接执行失败时检查执行权限、shebang 和路径；只写文件名找不到时检查 `PATH`；新终端和当前终端行为不一致时检查临时 `export` 与 `~/.bashrc` 的区别。

---

## 12. 本阶段完成后应当能解释

完成第一次培训任务后，至少应该能结合自己做过的操作说明：

- 绝对路径、相对路径、`~`、`.`、`..` 分别是什么；
- 为什么 `python3 a.py` 成功不代表 `./a.py` 一定成功；
- 执行权限与 shebang 各解决什么问题；
- 为什么 `./a.py` 和直接输入 `a.py` 的查找方式不同；
- PATH 是什么，`~/.bashrc` 又在什么时候发挥作用；
- 为什么任务最后要恢复 `.bashrc` 和环境变量；
- 本地 deb 安装时为什么要关心 CPU 架构；
- `/dev/tty*` 为什么和后续机器人串口设备有关；
- 出错时怎样先判断是路径、权限、环境还是程序本身的问题。

如果这些问题能结合任务过程解释清楚，这一阶段的 Linux 基础就已经达到了后续培训所需要的程度。
