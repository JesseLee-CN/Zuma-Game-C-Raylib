# Zuma Game — Architecture Reference

## Overview

GPU-accelerated Zuma-style tile-matching game. Forked from [Zuma-Game-C-EasyX](https://github.com/JesseLee-CN/Zuma-Game-C-EasyX), with the EasyX (GDI/CPU) rendering backend replaced by **raylib 6.0 (OpenGL 3.3+ / GPU)**. All game logic is preserved — state machine, collision detection, linked-list elimination, Archimedean spiral computation.

## Architecture

```
main.cpp (game logic, no rendering calls)
    └── render.h (abstraction layer)
        └── render.cpp (raylib backend)
            └── raylib 6.x (GPU)
```

### Three-layer separation

| Layer | Files | Responsibility |
|-------|-------|----------------|
| **Game Logic** | `main.cpp`, `LinkList.cpp/.h`, `ball.h` | State machine, physics, data structures |
| **Render Abstraction** | `render.h`, `render.cpp` | Thin wrapper around raylib (~130 lines) |
| **GPU Backend** | raylib 6.x (external) | OpenGL 3.3+ hardware rasterization, text atlas, VSync |

The abstraction layer maps 22 original EasyX API calls to 17 render functions. Switching backends requires changing only `render.cpp`.

## Key design decisions

- **raylib over SDL2/Direct2D/SFML/Raw OpenGL**: API mirrors EasyX closely (imperative draw calls, no scene graph), native MinGW-w64 support, built-in shapes and text.
- **Coordinate conversion**: EasyX uses diagonal corners `(x1,y1,x2,y2)` for rectangles; the wrapper converts to raylib's `(x, y, w, h)`.
- **Input model**: Changed from EasyX message-driven (`MouseHit`/`GetMouseMsg`) to raylib immediate-mode polling (`IsMouseButtonDown`/`Released`). Aiming mechanic (hold to aim, release to fire) is preserved.
- **Font metrics**: EasyX `textheight` ≈ raylib `fontSize`; centering uses `MeasureText` for width.
- **Rounded rects**: EasyX uses pixel radius; raylib uses 0–1 roundness fraction. Converted via `roundness = radius / (min(w,h) / 2)`.

## Build

### Prerequisites
- Windows, MinGW-w64 g++
- [raylib 6.0+](https://github.com/raysan5/raylib/releases) (win64_mingw-w64)
- Set `RAYLIB_PATH` environment variable to raylib directory

### Commands
```bash
make build RAYLIB_PATH="D:/raylib/raylib-6.0_win64_mingw-w64"
make run
make clean
```

Or via VS Code: `Ctrl+Shift+B`.

### Link order
`-lraylib -lopengl32 -lgdi32 -lwinmm` (raylib must precede system libs).

## Scoring

See `docs/rules.md` for the full mathematical model. Summary:

- Base score: `N² × 10` (N ≥ 3 consecutive same-color balls)
- Chain multiplier: `1 + (k − 1) × 0.5` (k = chain round)
- One-shot elimination scores higher than the same balls split across multiple chains.
