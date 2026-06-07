#include "render.h"
#include <stdarg.h>
#include <stdio.h>

// ── Color table for the 6 ball types ────────────────
Color ballColorTable[6] = {
	BLUE, GREEN, RED, YELLOW, MAGENTA, BROWN
};

// ── Window ──────────────────────────────────────────
void RenderInit(int width, int height, const char* title)
{
	SetConfigFlags(FLAG_WINDOW_RESIZABLE);
	InitWindow(width, height, title);
	SetTargetFPS(60);
}

void RenderClose(void)
{
	CloseWindow();
}

bool RenderShouldClose(void)
{
	return WindowShouldClose();
}

int GetRenderW(void)
{
	return GetScreenWidth();
}

int GetRenderH(void)
{
	return GetScreenHeight();
}

// ── Frame ───────────────────────────────────────────
void BeginRender(Color clearColor)
{
	BeginDrawing();
	ClearBackground(clearColor);
}

void EndRender(void)
{
	EndDrawing();
}

// ── Shapes ──────────────────────────────────────────
void DrawFilledCircle(float x, float y, float r, Color c)
{
	DrawCircleV((Vector2){x, y}, r, c);
}

void DrawFilledRectRounded(float x, float y, float w, float h,
                           float roundness, Color c)
{
	DrawRectangleRounded((Rectangle){x, y, w, h}, roundness, 8, c);
}

void DrawRectRoundedLines(float x, float y, float w, float h,
                          float roundness, Color c)
{
	DrawRectangleRoundedLinesEx((Rectangle){x, y, w, h}, roundness, 8, 1.0f, c);
}

void DrawWideLine(float x1, float y1, float x2, float y2, Color c)
{
	DrawLineEx((Vector2){x1, y1}, (Vector2){x2, y2}, 1.0f, c);
}

// ── Text ────────────────────────────────────────────
void DrawTextStr(const char* text, float x, float y, int fontSize, Color c)
{
	DrawText(text, (int)x, (int)y, fontSize, c);
}

void DrawTextFmt(float x, float y, int fontSize, Color c, const char* fmt, ...)
{
	char buf[128];
	va_list args;
	va_start(args, fmt);
	vsnprintf(buf, sizeof(buf), fmt, args);
	va_end(args);
	DrawText(buf, (int)x, (int)y, fontSize, c);
}

int MeasureTextStr(const char* text, int fontSize)
{
	return MeasureText(text, fontSize);
}

// ── Input ───────────────────────────────────────────
Vector2 GetInputMousePos(void)
{
	return GetMousePosition();
}

bool IsInputLeftPressed(void)
{
	return IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

bool IsInputLeftReleased(void)
{
	return IsMouseButtonReleased(MOUSE_BUTTON_LEFT);
}

bool IsInputLeftDown(void)
{
	return IsMouseButtonDown(MOUSE_BUTTON_LEFT);
}

bool IsInputRightPressed(void)
{
	return IsMouseButtonPressed(MOUSE_BUTTON_RIGHT);
}

bool IsInputReady(void)
{
	return true;
}

// ── Utility ─────────────────────────────────────────
bool PointInRect(int mx, int my, int x, int y, int w, int h)
{
	return mx >= x && mx <= x + w && my >= y && my <= y + h;
}
