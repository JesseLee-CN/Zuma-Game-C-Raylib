# Zuma-Game-C-Raylib

基于 C++ 与 Raylib (OpenGL GPU 加速) 的祖玛游戏实现

## 项目概述

本项目是 [Zuma-Game-C-EasyX](https://github.com/JesseLee-CN/Zuma-Game-C-EasyX) 的 GPU 渲染重构版本。将原有的 EasyX (GDI/CPU 渲染) 后端替换为 **Raylib (OpenGL 3.3+ / GPU 渲染)**，同时保持全部游戏逻辑不变。

### 重构目标
- GPU 加速 2D 渲染（硬件光栅化、纹理文字、自动批处理）
- 稳定的 60 FPS（原 GDI 版本在球数较多时掉帧）
- 渲染抽象层分离，后端可替换（详见 [PLAN.md](PLAN.md)）

## 核心架构

```
main.cpp (游戏逻辑)
    └── render.h (渲染抽象层)
        └── render.cpp (Raylib OpenGL 后端)
            └── raylib 5.x (GPU)
```

游戏逻辑层（状态机、碰撞检测、阿基米德螺旋计算、双向链表消除算法）**完全独立于渲染后端**。

## 环境要求

- Windows 系统（MinGW-w64 g++）
- [Raylib 5.5+](https://github.com/raysan5/raylib/releases)

### 安装 Raylib

1. 下载 `raylib-5.5_win64_mingw-w64.zip`
2. 解压到 `C:\raylib\`（或自定义路径）
3. 设置环境变量：`RAYLIB_PATH=C:\raylib`

验证安装：
```bash
ls $RAYLIB_PATH/include/raylib.h
ls $RAYLIB_PATH/lib/libraylib.a
```

## 构建

### VS Code（推荐）
在 VS Code 中打开项目，按 `Ctrl+Shift+B` 构建。

### 命令行
```bash
# 使用 Makefile
make build

# 或手动编译
g++ -g -Wall -Wextra -std=c++17 -mwindows \
    -Iinclude -I${RAYLIB_PATH}/include \
    -L${RAYLIB_PATH}/lib \
    src/main.cpp src/LinkList.cpp src/render.cpp \
    -o build/ZumaGame.exe \
    -lraylib -lopengl32 -lgdi32 -lwinmm
```

**链接顺序重要：** `-lraylib` 必须在 `-lopengl32 -lgdi32 -lwinmm` 之前。

### 运行
```bash
make run
# 或
./build/ZumaGame.exe
```

## 项目结构

```
Zuma-Game-C-Raylib/
├── PLAN.md              # 完整重构方案文档
├── Makefile             # 构建脚本
├── .gitignore
├── .vscode/
│   └── tasks.json       # VS Code 构建任务
├── docs/
│   └── rules.md         # 计分规则
├── include/
│   ├── ball.h           # 球结构体（无图形依赖）
│   ├── LinkList.h       # 双向链表接口
│   └── render.h         # 渲染抽象层接口 [新]
├── src/
│   ├── main.cpp         # 游戏主逻辑（使用 render.h）
│   ├── LinkList.cpp     # 链表实现（不变）
│   └── render.cpp       # Raylib 渲染后端 [新]
└── build/               # 构建输出目录
```

## 游戏说明

见原项目 README。操作方式、难度系统、计分规则保持不变。

### 操作

| 操作 | 功能 |
|------|------|
| 鼠标移动 | 瞄准 / 菜单悬停 |
| 鼠标左键 | 菜单选择 / 按住瞄准松开发射 |
| 鼠标右键 | 任意界面退出 |
| 窗口缩放 | 自由拖动边框 |
