# Git 基础

这份资料服务于整个四足培训期间的代码提交和版本记录。它不是 Git 命令大全；先把培训中每天都会用到的工作流弄清楚，再在真正遇到复杂协作时学习 rebase、cherry-pick 等操作。

培训阶段最重要的是能够自己完成：

```text
查看修改 -> 确认差异 -> 暂存 -> 提交 -> 同步到 GitHub
```

以及在做错时知道自己目前改了什么，而不是第一反应删目录重新来。

---

## 1. Git 和 GitHub 不是一回事

Git 是版本控制系统，运行在本地。GitHub 是托管 Git 仓库并提供协作功能的网站。

没有 GitHub，也可以：

```bash
git init
git add
git commit
git log
```

只是历史保存在本机。

有了 GitHub，可以把本地提交推到远程，用于同步、备份、代码评审和多人协作。

培训中可以简单理解成：

```text
Git    管版本历史
GitHub 放远程仓库并协作
```

---

## 2. Git 最需要先理解的三个位置

在一个仓库中，先把这三个状态分开：

```text
工作区 -> 暂存区 -> commit 历史
```

### 工作区

就是你正在编辑的真实文件。

### 暂存区

保存“下一次 commit 准备记录哪些修改”。

### commit

把暂存区当前内容记录为一个新的版本。

因此：

```bash
git add 文件
```

并不是“把文件上传到 GitHub”；它只是把当前修改放进暂存区。

```bash
git commit -m "说明"
```

也不会自动上传；它只是创建本地提交。

真正向远程同步通常还要：

```bash
git push
```

---

## 3. 每次修改最常用的循环

进入仓库后第一件事可以先看：

```bash
git status
```

它会告诉你：

- 当前分支；
- 哪些文件被修改；
- 哪些是新文件；
- 哪些修改已经进入暂存区。

提交之前再看实际差异：

```bash
git diff
```

已经 `git add` 的内容可以看：

```bash
git diff --staged
```

然后只暂存本次真正要提交的文件：

```bash
git add README.md src/main.cpp
```

再次：

```bash
git status
git diff --staged
```

确认后：

```bash
git commit -m "完成基础仿真循环"
```

最后：

```bash
git push
```

这个循环比背几十条 Git 命令重要得多。

---

## 4. clone、pull、push 分别做什么

已有远程仓库时：

```bash
git clone <仓库地址>
```

会把仓库与历史复制到本地，并建立远程 `origin`。

查看远程：

```bash
git remote -v
```

### git pull

常用来获取远程更新并整合到当前分支。

在多人协作或者多台电脑切换时，开始工作前通常先确认：

```bash
git status
git pull
```

不要在自己有一堆未确认本地修改时盲目 pull；先看 status，知道自己当前状态。

### git push

把本地已经存在的 commit 推到远程。

如果只是修改了文件却没有 commit，`git push` 不会帮你自动记录这些工作区修改。

---

## 5. 第一次连接 GitHub：推荐理解 SSH

Linux 开发环境长期使用 GitHub 时，SSH 比反复输入凭据方便。

先看是否已有密钥：

```bash
ls -al ~/.ssh
```

没有合适密钥时可以生成：

```bash
ssh-keygen -t ed25519 -C "你的邮箱"
```

公钥通常是：

```text
~/.ssh/id_ed25519.pub
```

私钥通常是：

```text
~/.ssh/id_ed25519
```

**只能把公钥内容加入 GitHub；私钥不要上传、不要提交到仓库、不要发给别人。**

测试：

```bash
ssh -T git@github.com
```

SSH 仓库地址类似：

```text
git@github.com:USER/REPO.git
```

HTTPS 也可以正常使用，但 GitHub 的 Git 操作不能再直接使用账户密码作为认证。若选择 HTTPS，应使用合适的 token / credential manager / GitHub CLI 等认证方式。

---

## 6. .gitignore：哪些东西不应该进仓库

构建目录、缓存和临时文件通常不值得进入版本历史。

例如 C++ 工程常见：

```gitignore
build/
*.o
```

Python 常见：

```gitignore
__pycache__/
*.pyc
```

还要特别避免提交：

```text
密码
Token
SSH 私钥
API Key
大体积无关数据
本地 IDE 缓存
```

`.gitignore` 只会自动忽略**尚未被 Git 跟踪**的匹配文件。如果文件已经提交过，仅仅后来写进 `.gitignore` 并不会自动从历史中消失。

---

## 7. 分支：把实验和稳定版本隔开

分支可以先理解为指向某条提交历史当前位置的名字。

查看：

```bash
git branch
```

创建并切换：

```bash
git switch -c experiment
```

回到主分支：

```bash
git switch main
```

如果实验已经完成并需要合回主分支：

```bash
git switch main
git merge experiment
```

培训初期不要求复杂分支策略，但当你准备做一项可能破坏当前可运行版本的实验时，开一个分支往往比复制出 `project_final2_backup` 更清楚。

---

## 8. 冲突不是“Git 坏了”

当两个分支修改了同一位置且 Git 无法自动判断如何合并时，会产生冲突。

先：

```bash
git status
```

冲突文件中可能出现：

```text
<<<<<<< HEAD
当前分支内容
=======
另一边内容
>>>>>>> other
```

这些标记不是最终代码。你需要人工决定保留什么、删掉标记，然后：

```bash
git add 冲突文件
git commit
```

不要看到冲突就随便 `git reset --hard`。先知道哪些修改是自己的、哪些已经提交、哪些还只在工作区。

---

## 9. 做错以后，先判断“错在哪一层”

### 文件改了但还没 add

先看：

```bash
git diff
```

如果明确要丢弃某个文件的工作区修改，可以使用：

```bash
git restore 文件
```

### 已经 add，但还没 commit

看：

```bash
git diff --staged
```

想把文件从暂存区拿回来、但保留工作区修改：

```bash
git restore --staged 文件
```

### 已经 commit

先：

```bash
git log --oneline
```

再决定应该新建一个修正 commit、revert，还是在尚未共享的个人分支上改写历史。

培训阶段一个很安全的原则是：**已经 push 并且别人可能基于它继续工作的历史，不要随意强行重写。**

`reset --hard`、强制 push 等命令不是不能用，而是在你明确知道会丢掉什么之前不要把它们当作日常撤销键。

---

## 10. 培训仓库推荐工作流

第一次建立个人培训仓库后，后续每次任务都在同一个仓库中继续。

例如：

```text
quadruped-training-xxx/
├── README.md
├── 01_linux/
├── 02_mujoco/
├── 03_motor/
└── ...
```

一次任务不要只留下最后一个 `final` commit。开发过程中可以按实际进展提交：

```text
建立基础目录
完成模型加载
加入平坦场景
设置初始姿态
整理运行说明
```

提交信息不需要模仿大型公司的格式，但应至少能让未来的自己知道这次改了什么。

提交前习惯性检查：

```bash
git status
git diff
git diff --staged
```

完成后：

```bash
git log --oneline --decorate -n 10
```

看看自己的历史是否真的能读懂。

---

## 11. 一个很小的自练习

不要拿正式任务仓库试危险操作。可以单独建一个临时目录：

```bash
mkdir git_practice
cd git_practice
git init -b main
```

自己完成下面几件事：

1. 新建 `note.txt`，写一行内容并提交；
2. 再修改一行，先用 `git diff` 看变化；
3. `git add` 后比较 `git diff` 与 `git diff --staged`；
4. 创建第二个 commit；
5. 用 `git log --oneline` 查看两次提交；
6. 创建 `experiment` 分支，再改一次文件；
7. 回到 `main`，观察工作区内容为什么变回主分支版本；
8. 合并 `experiment`。

如果这些步骤都能自己做出来，Git 的最小工作模型已经建立了。

---

## 12. 培训阶段先掌握这些就够用

必须熟练：

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
merge 与冲突
restore 的基本用法
```

rebase、cherry-pick、reflog、bisect、submodule 等工具以后遇到真实需求再学。Git 不是靠一次背完整命令表掌握的，而是在每次提交前都知道“现在有哪些修改、下一次 commit 到底会记录什么”。
