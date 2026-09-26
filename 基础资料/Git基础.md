# Git 基础

这份资料用于培训期间的代码管理。先掌握日常提交、同步和恢复操作，rebase、cherry-pick、reflog 等工具以后遇到具体需求再学。

常用工作流：

```text
查看修改
  ↓
确认差异
  ↓
暂存
  ↓
提交
  ↓
推送到 GitHub
```

## 安装和身份配置

检查 Git：

```bash
git --version
```

Ubuntu / Debian 中安装：

```bash
sudo apt update
sudo apt install git
```

设置提交者姓名和邮箱：

```bash
git config --global user.name "Your Name"
git config --global user.email "you@example.com"
```

统一新仓库默认分支名：

```bash
git config --global init.defaultBranch main
```

查看当前配置：

```bash
git config --list
```

WSL 中的 Git 配置与 Windows 主机互相独立。Windows 已经配置过 Git 或 SSH，并不会自动同步到 WSL。

## Git 与 GitHub

Git 负责本地版本历史。GitHub 用来托管远程 Git 仓库，并提供协作、Pull Request、Issue 等功能。

本地没有连接 GitHub 时，下面这些命令仍然可以正常使用：

```bash
git init
git add
git commit
git log
```

连接 GitHub 后，可以把本地 commit 推到远程。

## 工作区、暂存区和 commit

一个仓库里可以先区分三个位置：

```text
工作区 -> 暂存区 -> commit 历史
```

### 工作区

当前磁盘上的文件，也就是正在编辑的内容。

### 暂存区

保存下一次 commit 准备记录的修改。

```bash
git add 文件
```

会把指定文件当前的修改加入暂存区。

### commit

```bash
git commit -m "说明"
```

把暂存区记录为一个新的本地版本。

commit 仍然位于本地。需要同步到 GitHub 时再执行：

```bash
git push
```

## 一次正常提交

先看状态：

```bash
git status
```

查看尚未暂存的修改：

```bash
git diff
```

暂存需要提交的文件：

```bash
git add README.md src/main.cpp
```

查看已经暂存的差异：

```bash
git diff --staged
```

确认以后提交：

```bash
git commit -m "完成基础仿真循环"
```

推送：

```bash
git push
```

提交前先看 `git status` 和 `git diff`，可以避免把无关文件一起提交。

## clone、remote、fetch、pull、push

克隆已有仓库：

```bash
git clone <仓库地址>
```

查看远程：

```bash
git remote -v
```

如果本地已经通过 `git init` 建好仓库，再连接一个新的 GitHub 空仓库：

```bash
git remote add origin git@github.com:USER/REPO.git
git push -u origin main
```

`-u` 会建立本地分支和远程分支的跟踪关系。

### fetch

```bash
git fetch origin
```

获取远程提交和分支信息，不自动修改当前分支。

### pull

```bash
git pull
```

获取远程更新并整合到当前分支。

执行前先看：

```bash
git status
```

如果工作区还有未处理的修改，先明确这些修改应该提交、暂存还是丢弃。

### push

```bash
git push
```

把本地已有的 commit 推到远程。工作区中尚未 commit 的文件不会被自动上传。

## 使用 SSH 连接 GitHub

检查已有密钥：

```bash
ls -al ~/.ssh
```

生成 Ed25519 密钥：

```bash
ssh-keygen -t ed25519 -C "你的邮箱"
```

常见文件：

```text
~/.ssh/id_ed25519       私钥
~/.ssh/id_ed25519.pub   公钥
```

把公钥加入 GitHub。私钥应一直保存在自己的机器上，不上传到仓库，也不要发送给别人。

测试连接：

```bash
ssh -T git@github.com
```

SSH 仓库地址通常形如：

```text
git@github.com:USER/REPO.git
```

HTTPS 也可以使用，需要通过 token、credential manager、GitHub CLI 等方式完成认证。

## .gitignore

构建产物、缓存和临时文件通常不提交到仓库。

C++ 常见：

```gitignore
build/
*.o
```

Python 常见：

```gitignore
__pycache__/
*.pyc
```

还应避免提交：

```text
密码
Token
SSH 私钥
API Key
IDE 缓存
可重新生成的大型构建产物
```

`.gitignore` 只影响尚未被 Git 跟踪的文件。已经被跟踪的文件需要先停止跟踪：

```bash
git rm --cached 文件
```

大型二进制文件如果确实需要版本化，可以再考虑 Git LFS。

## 分支

查看分支：

```bash
git branch
```

创建并切换分支：

```bash
git switch -c experiment
```

回到主分支：

```bash
git switch main
```

合并：

```bash
git switch main
git merge experiment
```

做可能破坏当前可运行版本的实验时，可以先建立独立分支。这样实验过程和主分支历史都比较清楚。

## 合并冲突

两个分支修改同一位置时，Git 可能无法自动合并。

先查看：

```bash
git status
```

冲突文件中会出现类似：

```text
<<<<<<< HEAD
当前分支内容
=======
另一边内容
>>>>>>> other
```

人工选择最终内容，删除冲突标记，然后：

```bash
git add 冲突文件
git commit
```

如果决定取消尚未完成的 merge：

```bash
git merge --abort
```

冲突期间不要直接使用 `git reset --hard` 清理现场，除非已经确认哪些未提交修改会被删除。

## 恢复修改

不同阶段使用不同命令。

### 工作区已经修改，还没有 add

查看：

```bash
git diff
```

丢弃某个文件的工作区修改：

```bash
git restore 文件
```

### 已经 add，还没有 commit

查看：

```bash
git diff --staged
```

移出暂存区并保留工作区修改：

```bash
git restore --staged 文件
```

### 已经 commit

先看历史：

```bash
git log --oneline
```

如果错误 commit 已经推送到公共分支，可以新建一个反向提交：

```bash
git revert <commit-id>
```

尚未共享的个人分支可以根据具体情况修改本地历史。使用 `reset --hard` 或强制 push 前，应先确认会删除哪些提交和工作区修改。

## 培训仓库

建议整个培训阶段使用同一个个人仓库，例如：

```text
quadruped-training-xxx/
├── README.md
├── 01_linux/
├── 02_mujoco/
├── 03_motor/
└── ...
```

一次任务可以按实际进度提交多次：

```text
建立基础目录
完成模型加载
加入平坦场景
设置初始姿态
整理运行说明
```

commit message 能说明这次修改内容即可。

提交前：

```bash
git status
git diff
git diff --staged
```

查看最近历史：

```bash
git log --oneline --decorate -n 10
```

## 自练习

单独建立一个练习仓库：

```bash
mkdir git_practice
cd git_practice
git init -b main
```

完成下面几步：

1. 新建 `note.txt`，写一行内容并提交；
2. 修改文件，用 `git diff` 查看变化；
3. `git add` 后使用 `git diff --staged`；
4. 创建第二个 commit；
5. 使用 `git log --oneline` 查看历史；
6. 建立 `experiment` 分支并修改文件；
7. 切回 `main`，观察工作区内容变化；
8. 合并 `experiment`。

## 培训阶段需要掌握的命令

经常使用：

```text
git clone
git status
git diff
git add
git commit
git pull
git push
git log
```

需要理解：

```text
.gitignore
工作区 / 暂存区 / commit
本地分支 / 远程分支
fetch / pull / push
merge 与冲突
restore
revert
```

rebase、cherry-pick、reflog、bisect、submodule 等内容留到后续实际项目中再学。
