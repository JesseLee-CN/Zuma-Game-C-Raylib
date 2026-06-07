# Zuma Game

基于 C++ 与 raylib 的祖玛消球游戏。

## 项目概述

本项目是数据结构课程项目，实现经典祖玛（Zuma）游戏的核心玩法：沿阿基米德螺旋线排列的彩球链逐渐向中心收缩，玩家瞄准并发射彩球，当 ≥3 个同色球连在一起时消除得分。游戏使用**双向链表**管理球链，支持动态插入与连锁消除。

## 环境要求

- Windows 系统（MinGW-w64 g++）
- [raylib 6.0+](https://github.com/raysan5/raylib/releases)

### 安装 raylib

1. 下载 `raylib-6.0_win64_mingw-w64.zip`
2. 解压到目标目录（如 `D:\EnviroConfig\raylib\raylib-6.0_win64_mingw-w64`）
3. 构建时指定 `RAYLIB_PATH` 指向该目录

## 构建与运行

### VS Code（推荐）

在 VS Code 中打开项目，按 `Ctrl+Shift+B` 构建。

### 命令行

```bash
make build RAYLIB_PATH="/path/to/raylib"
make run   RAYLIB_PATH="/path/to/raylib"
make clean
```

### 手动编译

```bash
g++ -g -Wall -Wextra -std=c++17 \
    -Iinclude -I${RAYLIB_PATH}/include \
    -L${RAYLIB_PATH}/lib \
    src/main.cpp src/LinkList.cpp src/render.cpp \
    -o build/ZumaGame.exe \
    -lraylib -lopengl32 -lgdi32 -lwinmm
```

**链接顺序**：`-lraylib` 必须在 `-lopengl32 -lgdi32 -lwinmm` 之前。

## 项目结构

```
Zuma-Game-C-Raylib/
├── Makefile
├── README.md
├── .gitignore
├── .vscode/
│   └── tasks.json         # VS Code 构建任务
├── docs/
│   └── rules.md           # 计分规则
├── include/
│   ├── ball.h             # 球结构体
│   ├── LinkList.h         # 双向链表接口
│   └── render.h           # 渲染抽象层接口
├── src/
│   ├── main.cpp           # 游戏主逻辑（状态机、碰撞检测、螺旋布局）
│   ├── LinkList.cpp       # 链表实现 + 消除算法 + 计分
│   └── render.cpp         # raylib 渲染后端
└── build/                 # 构建输出（被 .gitignore 忽略）
```

## 架构

```
main.cpp (游戏逻辑)
    └── render.h (渲染抽象层)
        └── render.cpp (raylib 后端)
```

游戏逻辑层（状态机、碰撞检测、螺旋计算、链表消除算法）独立于渲染后端，通过 `render.h` 抽象接口与图形层通信。

## 操作

| 操作     | 功能                       |
| -------- | -------------------------- |
| 鼠标移动 | 瞄准 / 菜单悬停            |
| 鼠标左键 | 菜单选择 / 按住瞄准松开发射 |
| 鼠标右键 | 任意界面退出               |
| 窗口缩放 | 自由拖动边框               |

## 难度

| 模式       | 初始球数 | 射击次数 |
| ---------- | -------- | -------- |
| INFINITE   | 10       | 无限     |
| EASY       | 10       | 100      |
| COMMON     | 20       | 80       |
| HARD       | 30       | 50       |
| CUSTOMIZED | 自定义   | 自定义   |

## 计分

详见 `docs/rules.md`。核心公式：

- 基础分 = N² × 10（N ≥ 3 个连续同色球）
- 连锁系数 = 1 + (k − 1) × 0.5
- 一次性消除比多次连锁总分更高
