# Zuma-Game-C-Raylib

基于 C++ 与 raylib 的祖玛消球游戏，由 [Zuma-Game-C-EasyX](https://github.com/JesseLee-CN/Zuma-Game-C-EasyX) 重构而来。

## 项目概述

本项目是数据结构课程项目，实现经典祖玛（Zuma）游戏的核心玩法：沿阿基米德螺旋线排列的彩球链逐渐向中心收缩，玩家瞄准并发射彩球，当 ≥3 个同色球连在一起时消除得分。游戏使用**双向链表**管理球链，支持动态插入与连锁消除。

原项目使用 EasyX 图形库，本重构将其替换为 raylib，同时保持全部游戏逻辑不变。

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
    src/main.cpp src/LinkList.cpp src/render.cpp src/game.cpp \
    -o build/ZumaGame.exe \
    -lraylib -lopengl32 -lgdi32 -lwinmm -mwindows
```

**链接顺序**：`-lraylib` 必须在 `-lopengl32 -lgdi32 -lwinmm` 之前。`-mwindows` 使程序以 Windows GUI 子系统运行，启动时不显示命令行窗口。

## 项目结构

```
Zuma-Game-C-Raylib/
├── Makefile
├── README.md
├── .gitignore
├── .vscode/
│   └── tasks.json            # VS Code 构建任务
├── docs/
│   └── rules.md              # 计分规则
├── include/
│   ├── ball.h                # 球结构体（含 theta、animTimer 动画字段）
│   ├── LinkList.h            # 双向链表接口
│   ├── game.h                # 游戏上下文与状态机
│   └── render.h              # 渲染抽象层接口
├── src/
│   ├── main.cpp              # 程序入口 + 主循环
│   ├── game.cpp              # 游戏逻辑（状态机、碰撞、螺旋、动画）
│   ├── LinkList.cpp          # 链表实现 + 消除算法 + 计分
│   └── render.cpp            # raylib 渲染后端
└── build/                    # 构建输出（被 .gitignore 忽略）
```

## 架构

```
main.cpp (主循环调度)
    ├── game.cpp (游戏逻辑层)
    │   ├── 状态机 (MENU/CUSTOMIZE/PLAYING/CLEARING/SETTLEMENT)
    │   ├── 碰撞检测 + 消除算法 + 计分
    │   ├── 螺旋布局 (updateBallPos)
    │   └── CLEARING 动画 (三阶段)
    ├── LinkList.cpp (双向链表)
    ├── render.h (渲染抽象层)
    │   └── render.cpp (raylib 后端)
    └── ball.h (基础数据类型)
```

游戏逻辑层独立于渲染后端，通过 `render.h` 抽象接口与图形层通信。

## 操作

| 操作     | 功能                       |
| -------- | -------------------------- |
| 鼠标移动 | 瞄准 / 菜单悬停            |
| 鼠标左键 | 菜单选择 / 按住瞄准松开发射 |
| 窗口缩放 | 自由拖动边框               |

### EXIT 按钮

| 场景    | 位置       | 功能                              |
| ------- | ---------- | --------------------------------- |
| MENU    | 难度列表下 | 直接退出游戏                      |
| PLAYING | 左上角     | 进入结算（保留剩余球数惩罚）      |
| SETTLEMENT | 底部   | 退出游戏                          |

## 难度

| 模式       | 初始球数 | 射击次数 |
| ---------- | -------- | -------- |
| INFINITE   | 10       | 无限     |
| EASY       | 10       | 100      |
| COMMON     | 20       | 80       |
| HARD       | 30       | 50       |
| CUSTOMIZED | 自定义   | 自定义   |

## 结算动画

游戏结束时进入 CLEARING 状态，分为三个阶段：

| 阶段 | 触发条件 | 动画效果 |
| ---- | -------- | -------- |
| Phase 0 | 有剩余球 | 球沿阿基米德螺旋线从最内层依次向中央移动并消失，每个球消失时弹出红色 `-20` 扣分动画。采用三段式缓动（加速→匀速→减速），多球可重叠运动。匀速速度为发射速度的 1.5 倍。 |
| Phase 1 | 所有情况 | 百叶窗式画面淡出：10 条水平黑带从上到下依次闭合覆盖画面 |
| Phase 2 | 所有情况 | Score 文字从右上角放大飞入中央，自适应窗口大小 |

通关（无剩余球）时跳过 Phase 0，直接进入百叶窗淡出。

## 计分

详见 `docs/rules.md`。核心公式：

- 基础分 = N² × 10（N ≥ 3 个连续同色球）
- 连锁系数 = 1 + (k − 1) × 0.5
- 一次性消除比多次连锁总分更高
- 剩余球惩罚：每个剩余球 −20 分（结算动画中逐球扣除）
