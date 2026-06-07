#pragma once
#include <raylib.h>

#ifdef __cplusplus
extern "C" {
#endif

// ── Window ──────────────────────────────────────────
void RenderInit(int width, int height, const char* title);
void RenderClose(void);
bool RenderShouldClose(void);
int  GetRenderW(void);
int  GetRenderH(void);

// ── Frame ───────────────────────────────────────────
void BeginRender(Color clearColor);
void EndRender(void);

// ── Shapes ──────────────────────────────────────────
void DrawFilledCircle(float x, float y, float r, Color c);
void DrawFilledRectRounded(float x, float y, float w, float h,
                           float roundness, Color c);
void DrawRectRoundedLines(float x, float y, float w, float h,
                          float roundness, Color c);
void DrawWideLine(float x1, float y1, float x2, float y2, Color c);

// ── Text ────────────────────────────────────────────
void DrawTextStr(const char* text, float x, float y,
                 int fontSize, Color c);
void DrawTextFmt(float x, float y, int fontSize,
                 Color c, const char* fmt, ...);
int  MeasureTextStr(const char* text, int fontSize);

// ── Input ───────────────────────────────────────────
Vector2 GetInputMousePos(void);
bool IsInputLeftPressed(void);
bool IsInputLeftReleased(void);
bool IsInputLeftDown(void);
bool IsInputRightPressed(void);
bool IsInputReady(void);

// ── Utility ─────────────────────────────────────────
bool PointInRect(int mx, int my, int x, int y, int w, int h);

// ── Global color table ──────────────────────────────
extern Color ballColorTable[6];

#ifdef __cplusplus
}
#endif
