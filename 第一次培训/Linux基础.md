# Linux 系统与基础操作

这份讲义配合第一次培训的 [任务.md](./任务.md) 使用。任务中的操作需要自己完成；这里主要解释相关概念和常见问题。

## 终端与 Shell

Ubuntu 中按 `Ctrl + Alt + T` 可以打开终端。

终端负责显示输入和输出，命令由 Shell 解释执行。Ubuntu 默认常见的是 Bash。

例如：

```bash
python3 hello.py
```

Bash 会先查找 `python3`，再把 `hello.py` 作为参数交给 Python。

而：

```bash
./hello.py
```

表示直接执行当前目录下的 `hello.py`。这种运行方式会涉及执行权限和 shebang，后面会专门介绍。

查看命令帮助：

```bash
command --help
man command
```

例如：

```bash
ls --help
man chmod
```

## 文件系统与路径

Linux 文件系统从根目录 `/` 开始。普通用户的个人目录通常位于：

```text
/home/用户名
```

Bash 中可以用 `~` 表示当前用户的 home 目录。

常见系统目录：

| 目录 | 常见用途 |
|---|---|
| `/home` | 普通用户的个人文件 |
| `/etc` | 系统和软件配置 |
| `/usr` | 程序、库和系统资源 |
| `/opt` | 第三方软件、SDK |
| `/dev` | 设备文件 |
| `/tmp` | 临时文件 |

常用命令：

```bash
pwd
ls
cd 目录
cd ..
mkdir 目录
touch 文件
```

其中：

- `.` 表示当前目录；
- `..` 表示父目录；
- `pwd` 显示当前目录。

### 绝对路径与相对路径

```text
/home/leo/robocon/learn
```

从根目录 `/` 开始，因此是绝对路径。

如果当前位于：

```text
/home/leo/robocon
```

那么：

```text
learn
./learn
```

都是相对路径。

```text
~/robocon/learn
```

会先由 Shell 把 `~` 展开成当前用户的 home 目录。

遇到“文件找不到”时，可以先执行：

```bash
pwd
ls -l
```

确认当前目录和文件位置。

## 文件与目录操作

创建多级目录：

```bash
mkdir -p a/b/c
```

复制文件：

```bash
cp source.txt copy.txt
```

复制目录：

```bash
cp -r dir1 dir2
```

移动或重命名：

```bash
mv old.txt new.txt
```

删除文件：

```bash
rm file.txt
```

查看目录内容：

```bash
ls
ls -l
ls -a
ls -la
```

Linux 中以 `.` 开头的文件通常属于隐藏文件，例如：

```text
.bashrc
.git
```

查看文本文件：

```bash
cat file.txt
less file.txt
head file.txt
tail file.txt
```

持续观察日志末尾：

```bash
tail -f log.txt
```

按名称查找文件：

```bash
find . -name "*.py"
```

`rm` 删除文件后通常没有桌面环境中的回收站。使用 `rm -rf` 前要确认路径，尤其不要在不清楚当前目录时直接执行。

## Python 文件的几种运行方式

先查看当前 Python：

```bash
python3 --version
which python3
```

`which python3` 会显示 Bash 实际找到的 Python 可执行文件。以后使用虚拟环境、ROS 2 或 CUDA 时，这个信息经常用于检查环境。

如果 VS Code 已配置命令行入口，可以在工程目录中执行：

```bash
code .
```

打开整个目录。

创建一个简单脚本：

```python
print("hello")
```

保存为 `demo.py`。

### 使用 Python 解释器运行

```bash
python3 demo.py
```

这时 `demo.py` 只需要能够被 Python 读取，本身不要求具有执行权限。

### 直接执行脚本

```bash
./demo.py
```

系统会直接执行这个文件。

先检查权限：

```bash
ls -l demo.py
```

给当前用户增加执行权限：

```bash
chmod u+x demo.py
```

脚本还需要指定解释器。Python 脚本常写：

```python
#!/usr/bin/env python3

print("hello")
```

第一行称为 shebang。系统根据它找到对应解释器。

因此下面两条命令走的是不同的执行路径：

```text
python3 demo.py
./demo.py
```

第一次培训任务会专门观察这个差别。

## PATH

即使 `./demo.py` 已经可以运行，直接输入：

```bash
demo.py
```

仍然可能提示找不到命令。

Bash 会在环境变量 `PATH` 指定的目录中查找命令。查看当前 PATH：

```bash
echo "$PATH"
```

查看一个命令来自哪里：

```bash
which python3
command -v python3
```

假设自己的脚本统一放在：

```text
/home/用户名/bin
```

可以临时加入 PATH：

```bash
export PATH="$HOME/bin:$PATH"
```

这项修改只影响当前 Shell。关闭终端后，新开的 Shell 不会自动继承这条临时命令。

## .bashrc

Bash 启动交互式 Shell 时通常会读取：

```text
~/.bashrc
```

因此可以把需要长期生效的 Bash 配置写在这里。

修改前建议先备份：

```bash
cp ~/.bashrc ~/.bashrc.backup
```

修改以后可以在当前终端重新加载：

```bash
source ~/.bashrc
```

查看文件末尾：

```bash
tail -n 20 ~/.bashrc
```

如果在 `.bashrc` 中写入：

```bash
echo "hello"
```

之后每次打开新的交互终端都会执行它。

第一次任务最后要求恢复对 `.bashrc` 和环境变量的修改。操作时要记录自己增加了哪些内容，恢复时只删除相应配置。

## Shell 脚本

Bash 脚本可以写成：

```bash
#!/usr/bin/env bash

echo "hello"
```

保存为 `demo.sh` 后，可以交给 Bash 运行：

```bash
bash demo.sh
```

也可以在增加执行权限后直接运行：

```bash
chmod u+x demo.sh
./demo.sh
```

直接执行时同样会使用 shebang 指定的解释器。

Python 脚本和 Bash 脚本在这一点上的处理方式一致。

## 软件安装与 CPU 架构

Ubuntu 常用 `apt` 管理软件包：

```bash
sudo apt update
sudo apt install 包名
```

安装当前目录下的 deb 文件：

```bash
sudo apt install ./package.deb
```

这里的 `./` 表示当前目录中的文件。

查看机器架构：

```bash
uname -m
dpkg --print-architecture
```

常见名称：

| 常见名称 | 含义 |
|---|---|
| `x86_64` / `amd64` | Intel / AMD 64 位平台 |
| `aarch64` / `arm64` | ARM 64 位平台 |

下载 deb 时要选择与当前系统匹配的架构，并使用可信来源。

## 通配符、grep 与设备文件

Shell 通配符可以匹配文件名：

```bash
ls /dev/tty*
```

这里的 `*` 由 Bash 展开。

也可以把 `ls` 的输出交给 `grep`：

```bash
ls /dev | grep 'tty'
```

这时 `grep` 处理的是文本。

`/dev` 下保存的是设备文件。机器人开发中常见：

| 设备名 | 常见情况 |
|---|---|
| `ttyS*` | 主机原生串口 |
| `ttyUSB*` | USB 转串口 |
| `ttyACM*` | USB CDC ACM 设备 |

插入设备后可以查看最近的内核消息：

```bash
dmesg | tail
```

查看设备文件权限：

```bash
ls -l /dev/ttyUSB0
```

以后接串口设备时还可能用到 `dialout` 用户组和 udev 规则。

## 进程

前台运行程序时，终端会等待程序结束。

查看当前进程：

```bash
ps
ps aux
```

筛选 Python 进程：

```bash
ps aux | grep python
```

动态查看进程和资源占用：

```bash
top
```

已知 PID 时可以发送终止信号：

```bash
kill PID
```

`Ctrl + C` 通常会向当前前台进程发送中断信号。

后面运行 MuJoCo、ROS 2 节点和训练程序时，会频繁接触进程管理。

## 排错顺序

路径相关问题先看：

```bash
pwd
ls -l
```

文件内容：

```bash
cat 文件名
head 文件名
```

脚本执行问题可以依次检查：

1. 当前执行的是 `python3 file.py`、`./file.py` 还是直接输入 `file.py`；
2. 文件是否具有执行权限；
3. shebang 是否正确；
4. PATH 中是否包含脚本所在目录；
5. 修改是否只在当前 Shell 中生效；
6. `.bashrc` 是否已经重新加载。

## 完成第一次培训后

至少应当能够说明：

- 绝对路径和相对路径的区别；
- `~`、`.`、`..` 的含义；
- `python3 a.py` 与 `./a.py` 的区别；
- 执行权限和 shebang 的作用；
- PATH 怎样影响命令查找；
- `.bashrc` 在什么时候被读取；
- deb 包为什么要匹配 CPU 架构；
- `/dev/tty*` 与串口设备的关系；
- 遇到脚本无法执行时应检查哪些内容。
