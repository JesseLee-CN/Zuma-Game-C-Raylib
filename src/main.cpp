#include <time.h>
#include <math.h>
#include <stdlib.h>
#include "render.h"
#include "LinkList.h"
#include "ball.h"

#define TOTAL_THETA (4 * PI)

#define MAX_POPUPS 8

struct ScorePopup {
	float x, y;
	int score;
	int life;
};

enum GameState { MENU, CUSTOMIZE, PLAYING, CLEARING, SETTLEMENT };

int BALLRADIUS;
int winWidth = 600, winHeight = 600;
int centerX, centerY;
double R_OUTER, R_INNER;

void recomputeDimensions()
{
	int ref = (winWidth < winHeight ? winWidth : winHeight) / 2;
	BALLRADIUS = ref * 10 / 300;
	if (BALLRADIUS < 4) BALLRADIUS = 4;
	R_OUTER = ref * 260.0 / 300.0;
	R_INNER = ref * 30.0 / 300.0;
	centerX = winWidth / 2;
	centerY = winHeight / 2;
}

// 初始化球链表
void initBallList(Node* head, int count)
{
	int i;
	ball b;

	for (i = 0; i < count; ++i) {
		b.c = i % 6;
		ListInsert(head, 0, b);
	}
}

// 沿螺旋线按弧长步进布置球（由外向内紧密排列）
void updateBallPos(Node* head)
{
	const double k = (R_OUTER - R_INNER) / TOTAL_THETA;
	const double step = 2.0 * BALLRADIUS;
	const double dtheta = 0.002;

	double theta = 0.0;
	Node* p = head;

	while (p->next != NULL) {
		p = p->next;
		double r = R_OUTER - k * theta;
		p->data.x = (float)(centerX + r * cos(theta));
		p->data.y = (float)(centerY + r * sin(theta));

		double arc = 0.0;
		while (arc < step) {
			double r_cur = R_OUTER - k * theta;
			double r_next = R_OUTER - k * (theta + dtheta);
			double ds_cur = sqrt(r_cur * r_cur + k * k);
			double ds_next = sqrt(r_next * r_next + k * k);
			double ds = (ds_cur + ds_next) / 2.0 * dtheta;

			if (arc + ds > step) {
				double remaining = step - arc;
				double rate = (ds_cur + ds_next) / 2.0;
				theta += remaining / rate;
				break;
			}

			arc += ds;
			theta += dtheta;
		}
	}
}

// 碰撞检测：找到发射球与球链的碰撞段，返回插入索引
bool collisionDetection(Node* head, ball b, int* id)
{
	int threshold = 2 * BALLRADIUS;
	int threshSq = threshold * threshold;

	Node* p = head->next;
	int index = 0;

	float best1 = 1e9f, best2 = 1e9f;
	int idx1 = -1, idx2 = -1;

	while (p != NULL) {
		float dx = p->data.x - b.x;
		float dy = p->data.y - b.y;
		float dist = dx * dx + dy * dy;

		if (dist < best1) {
			best2 = best1; idx2 = idx1;
			best1 = dist;  idx1 = index;
		} else if (dist < best2) {
			best2 = dist;  idx2 = index;
		}

		p = p->next;
		index++;
	}

	if (idx1 < 0 || best1 > threshSq)
		return false;

	if (idx2 >= 0 && (idx1 == idx2 + 1 || idx2 == idx1 + 1))
		*id = (idx1 > idx2) ? idx1 : idx2;
	else
		*id = idx1;

	return true;
}

// 绘制球链
void drawBallList(Node* head)
{
	Node* p = head;
	while (p->next != NULL) {
		p = p->next;
		DrawFilledCircle(p->data.x, p->data.y, (float)BALLRADIUS,
		                 ballColorTable[p->data.c]);
	}
}

// 绘制碰撞球
void drawColBall(ball* b, float x, float y)
{
	b->x = x;
	b->y = y;
	DrawFilledCircle(b->x, b->y, (float)BALLRADIUS, ballColorTable[b->c]);
}

// 绘制螺旋辅助线（灰白色虚线）
void drawSpiralGuide()
{
	const double k = (R_OUTER - R_INNER) / TOTAL_THETA;
	const double dtheta = 0.02;
	const double dashLen = BALLRADIUS * 1.4;
	const double gapLen = BALLRADIUS * 0.8;

	Color guideColor = {160, 160, 160, 255};

	double theta = 0.0;
	double r = R_OUTER;
	float prev_x = (float)(centerX + r * cos(theta));
	float prev_y = (float)(centerY + r * sin(theta));

	double segArc = 0.0;
	bool drawing = true;

	while (theta <= TOTAL_THETA + 0.5) {
		theta += dtheta;
		r = R_OUTER - k * theta;
		if (r < R_INNER) break;
		float x = (float)(centerX + r * cos(theta));
		float y = (float)(centerY + r * sin(theta));

		double dx = (double)(x - prev_x);
		double dy = (double)(y - prev_y);
		double segLen = sqrt(dx * dx + dy * dy);
		segArc += segLen;

		if (drawing) {
			DrawWideLine(prev_x, prev_y, x, y, guideColor);
			if (segArc >= dashLen) {
				drawing = false;
				segArc = 0.0;
			}
		} else {
			if (segArc >= gapLen) {
				drawing = true;
				segArc = 0.0;
			}
		}

		prev_x = x;
		prev_y = y;
	}
}

void drawButton(int x, int y, int w, int h, const char* text, int fontH, bool hovered)
{
	Color fillColor = hovered ? (Color){80, 80, 80, 255} : (Color){40, 40, 40, 255};
	Color lineColor = hovered ? WHITE : (Color){180, 180, 180, 255};
	float rad = (float)(h / 8);
	if (rad < 4.0f) rad = 4.0f;
	float roundness = rad / ((h < w ? h : w) / 2.0f);
	if (roundness > 1.0f) roundness = 1.0f;

	DrawFilledRectRounded((float)x, (float)y, (float)w, (float)h,
	                      roundness, fillColor);
	DrawRectRoundedLines((float)x, (float)y, (float)w, (float)h,
	                     roundness, lineColor);

	Color textColor = hovered ? WHITE : (Color){200, 200, 200, 255};
	int tw = MeasureTextStr(text, fontH);
	DrawTextStr(text, (float)(x + (w - tw) / 2),
	            (float)(y + (h - fontH) / 2), fontH, textColor);
}

int main()
{
	RenderInit(winWidth, winHeight, "ZUMA");
	recomputeDimensions();

	// ---- Game state variables ----
	GameState state = MENU;
	bool gameWon = false;
	int clearFrame = 0;

	// Difficulty configuration
	int startBalls = 10;
	int startShots = -1;   // -1 = infinite

	// Customize page values
	int customBalls = 20;
	int customShots = 80;
	bool customShotsInfinite = false;

	// Game objects (allocated when entering PLAYING)
	Node* head = NULL;
	ball cball;

	bool ballMoving = false;
	bool aiming = false;
	int aimx = 0, aimy = 0;
	float vx = 0, vy = 0;
	int counter = 0;
	int totalScore = 0;
	ScorePopup popups[MAX_POPUPS];
	int popupCount = 0;
	int remainingShots = -1;

	srand((unsigned int)time(NULL));

	while (!RenderShouldClose())
	{
		// ---- Window resize detection ----
		int newW = GetRenderW();
		int newH = GetRenderH();
		if (newW >= 200 && newH >= 200 && (newW != winWidth || newH != winHeight)) {
			int oldCX = centerX, oldCY = centerY, oldBR = BALLRADIUS;
			winWidth = newW;
			winHeight = newH;
			recomputeDimensions();
			if (state == PLAYING && head != NULL)
				updateBallPos(head);
			if (state == PLAYING && ballMoving) {
				float scale = (float)BALLRADIUS / (float)oldBR;
				cball.x = centerX + (cball.x - oldCX) * scale;
				cball.y = centerY + (cball.y - oldCY) * scale;
				vx *= scale;
				vy *= scale;
			}
			aimx = centerX + BALLRADIUS * 5;
			aimy = centerY;
		}

		counter++;

		// ---- Input handling (state-dependent) ----
		Vector2 mousePos = GetInputMousePos();
		int mx = (int)mousePos.x;
		int my = (int)mousePos.y;

		if (state == MENU) {
			aimx = mx; aimy = my;

			if (IsInputLeftReleased()) {
				int btnW = winWidth * 55 / 100;
				if (btnW < 200) btnW = 200;
				int btnH = winHeight * 10 / 100;
				if (btnH < 40) btnH = 40;
				int gap = btnH + 6;
				int startY = winHeight * 3 / 10;
				int btnX = centerX - btnW / 2;

				struct { const char* name; int balls; int shots; } diffs[] = {
					{"INFINITE",  10,  -1},
					{"EASY",      10, 100},
					{"COMMON",    20,  80},
					{"HARD",      30,  50},
					{"CUSTOMIZED", 0,   0},
				};

				for (int i = 0; i < 5; i++) {
					int by = startY + i * gap;
					if (PointInRect(mx, my, btnX, by, btnW, btnH)) {
						if (i == 4) {
							state = CUSTOMIZE;
						} else {
							startBalls = diffs[i].balls;
							startShots = diffs[i].shots;
							head = CreateEmptyList();
							initBallList(head, startBalls);
							updateBallPos(head);
							cball.c = rand() % 6;
							cball.x = (float)centerX;
							cball.y = (float)centerY;
							ballMoving = false;
							aiming = false;
							totalScore = 0;
							popupCount = 0;
							remainingShots = startShots;
							gameWon = false;
							state = PLAYING;
						}
						break;
					}
				}
			}
			if (IsInputRightPressed()) {
				RenderClose();
				return 0;
			}
		}
		else if (state == CUSTOMIZE) {
			aimx = mx; aimy = my;

			if (IsInputLeftReleased()) {
				int ctrlBtnS = winHeight / 20;
				if (ctrlBtnS < 22) ctrlBtnS = 22;
				if (ctrlBtnS > 36) ctrlBtnS = 36;
				int ctrlFontH = ctrlBtnS * 3 / 4;
				int rowH = ctrlBtnS + 8;
				int row1Y = winHeight * 35 / 100;
				int row2Y = row1Y + rowH + 12;

				int labelW = MeasureTextStr("Shots:  ", ctrlFontH) + 10;
				int valW = MeasureTextStr("200", ctrlFontH) + 30;
				int rowCenterX = centerX;

				int ballsMinusX = rowCenterX - labelW/2;
				int ballsValX  = ballsMinusX + ctrlBtnS + 4;
				int ballsPlusX = ballsValX + valW + 4;
				int shotsMinusX = rowCenterX - labelW/2;
				int shotsValX  = shotsMinusX + ctrlBtnS + 4;
				int shotsPlusX = shotsValX + valW + 4;
				int shotsInfX  = shotsPlusX + ctrlBtnS + 8;

				int startBtnW = winWidth * 30 / 100;
				if (startBtnW < 120) startBtnW = 120;
				int startBtnH = winHeight * 8 / 100;
				if (startBtnH < 32) startBtnH = 32;
				int startBtnX = centerX - startBtnW / 2;
				int startBtnY = winHeight * 65 / 100;
				int backBtnY = startBtnY + startBtnH + 8;

				if (PointInRect(mx, my, ballsMinusX, row1Y, ctrlBtnS, ctrlBtnS)) {
					if (customBalls > 5) customBalls--;
				}
				else if (PointInRect(mx, my, ballsPlusX, row1Y, ctrlBtnS, ctrlBtnS)) {
					if (customBalls < 50) customBalls++;
				}
				else if (!customShotsInfinite && PointInRect(mx, my, shotsMinusX, row2Y, ctrlBtnS, ctrlBtnS)) {
					if (customShots > 10) customShots--;
				}
				else if (!customShotsInfinite && PointInRect(mx, my, shotsPlusX, row2Y, ctrlBtnS, ctrlBtnS)) {
					if (customShots < 200) customShots++;
				}
				else if (PointInRect(mx, my, shotsInfX, row2Y, ctrlBtnS + 6, ctrlBtnS)) {
					customShotsInfinite = !customShotsInfinite;
				}
				else if (PointInRect(mx, my, startBtnX, startBtnY, startBtnW, startBtnH)) {
					startBalls = customBalls;
					startShots = customShotsInfinite ? -1 : customShots;
					head = CreateEmptyList();
					initBallList(head, startBalls);
					updateBallPos(head);
					cball.c = rand() % 6;
					cball.x = (float)centerX;
					cball.y = (float)centerY;
					ballMoving = false;
					aiming = false;
					totalScore = 0;
					popupCount = 0;
					remainingShots = startShots;
					gameWon = false;
					state = PLAYING;
				}
				else if (PointInRect(mx, my, startBtnX, backBtnY, startBtnW, startBtnH)) {
					state = MENU;
				}
			}
			if (IsInputRightPressed()) {
				RenderClose();
				return 0;
			}
		}
		else if (state == SETTLEMENT) {
			aimx = mx; aimy = my;

			if (IsInputLeftReleased()) {
				int btnW = winWidth * 3 / 10;
				if (btnW < 120) btnW = 120;
				int btnH = winHeight * 8 / 100;
				if (btnH < 36) btnH = 36;
				int btnSpacing = btnH / 2;
				int btnX = centerX - btnW / 2;
				int tryAgainY = centerY + winHeight / 6;
				int exitY = tryAgainY + btnH + btnSpacing;

				if (PointInRect(mx, my, btnX, tryAgainY, btnW, btnH)) {
					DestroyList(head);
					head = NULL;
					state = MENU;
				}
				else if (PointInRect(mx, my, btnX, exitY, btnW, btnH)) {
					DestroyList(head);
					RenderClose();
					return 0;
				}
			}
			if (IsInputRightPressed()) {
				DestroyList(head);
				RenderClose();
				return 0;
			}
		}
		else if (state == PLAYING) {
			aimx = mx; aimy = my;

			if (!ballMoving) {
				if (IsInputLeftDown()) {
					aiming = true;
				}
				if (IsInputLeftReleased() && aiming) {
					float dx = (float)(mx - centerX);
					float dy = (float)(centerY - my);
					float length = sqrtf(dx * dx + dy * dy);
					if (length > 0) {
						vx = (dx / length) * BALLRADIUS;
						vy = (dy / length) * BALLRADIUS;
					}
					if (remainingShots > 0)
						remainingShots--;
					ballMoving = true;
					aiming = false;
				}
			}

			if (IsInputRightPressed()) {
				DestroyList(head);
				RenderClose();
				return 0;
			}
		}
		// CLEARING: ignore all input

		// ---- Rendering (state-dependent) ----
		BeginRender(BLACK);

		switch (state) {

		// ==================== MENU ====================
		case MENU:
		{
			int titleFontH = winHeight / 8;
			if (titleFontH < 24) titleFontH = 24;
			DrawTextStr("ZUMA",
			            (float)(centerX - MeasureTextStr("ZUMA", titleFontH) / 2),
			            (float)(winHeight / 12),
			            titleFontH, (Color){255, 200, 50, 255});

			struct { const char* name; const char* desc; } diffs[] = {
				{"INFINITE",   "Balls: 10   |  Shots: Infinite"},
				{"EASY",       "Balls: 10   |  Shots: 100"},
				{"COMMON",     "Balls: 20   |  Shots: 80"},
				{"HARD",       "Balls: 30   |  Shots: 50"},
				{"CUSTOMIZED", "Set your own rules"},
			};

			int btnW = winWidth * 55 / 100;
			if (btnW < 200) btnW = 200;
			int btnH = winHeight * 10 / 100;
			if (btnH < 40) btnH = 40;
			int gap = btnH + 6;
			int startY = winHeight * 3 / 10;

			for (int i = 0; i < 5; i++) {
				int btnX = centerX - btnW / 2;
				int btnY = startY + i * gap;
				bool hover = PointInRect(aimx, aimy, btnX, btnY, btnW, btnH);

				Color btnFill = hover ? (Color){60, 60, 80, 255} : (Color){30, 30, 50, 255};
				Color btnLine = hover ? WHITE : (Color){150, 150, 200, 255};
				float rad = 10.0f;
				float roundness = rad / ((btnH < btnW ? btnH : btnW) / 2.0f);
				if (roundness > 1.0f) roundness = 1.0f;
				DrawFilledRectRounded((float)btnX, (float)btnY,
				                      (float)btnW, (float)btnH,
				                      roundness, btnFill);
				DrawRectRoundedLines((float)btnX, (float)btnY,
				                     (float)btnW, (float)btnH,
				                     roundness, btnLine);

				int nameFontH = btnH * 55 / 100;
				Color nameColor = hover ? WHITE : (Color){220, 220, 255, 255};
				int ntw = MeasureTextStr(diffs[i].name, nameFontH);
				DrawTextStr(diffs[i].name,
				            (float)(btnX + (btnW - ntw) / 2),
				            (float)(btnY + btnH * 8 / 100),
				            nameFontH, nameColor);

				int descFontH = btnH * 30 / 100;
				if (descFontH < 9) descFontH = 9;
				Color descColor = hover ? (Color){200, 200, 255, 255} : (Color){140, 140, 180, 255};
				int dtw = MeasureTextStr(diffs[i].desc, descFontH);
				DrawTextStr(diffs[i].desc,
				            (float)(btnX + (btnW - dtw) / 2),
				            (float)(btnY + btnH * 62 / 100),
				            descFontH, descColor);
			}
			break;
		}

		// ==================== CUSTOMIZE ====================
		case CUSTOMIZE:
		{
			int titleFontH = winHeight / 10;
			if (titleFontH < 20) titleFontH = 20;
			const char* title = "CUSTOMIZED";
			int ttw = MeasureTextStr(title, titleFontH);
			DrawTextStr(title, (float)(centerX - ttw / 2),
			            (float)(winHeight / 12), titleFontH,
			            (Color){200, 200, 255, 255});

			int ctrlBtnS = winHeight / 20;
			if (ctrlBtnS < 22) ctrlBtnS = 22;
			if (ctrlBtnS > 36) ctrlBtnS = 36;
			int ctrlFontH = ctrlBtnS * 3 / 4;
			int rowH = ctrlBtnS + 8;
			int row1Y = winHeight * 35 / 100;
			int row2Y = row1Y + rowH + 12;

			int labelW = MeasureTextStr("Shots:  ", ctrlFontH) + 10;
			int valW = MeasureTextStr("200", ctrlFontH) + 30;
			int rowCenterX = centerX;

			// ---- Row 1: Balls ----
			{
				int minusX = rowCenterX - labelW/2;
				int valX   = minusX + ctrlBtnS + 4;
				int plusX  = valX + valW + 4;
				int ctrlY  = row1Y;

				DrawTextStr("Balls:", (float)(rowCenterX - labelW/2 - MeasureTextStr("Balls: ", ctrlFontH) - 10),
				            (float)(ctrlY + 2), ctrlFontH, (Color){200, 200, 200, 255});

				bool hMinus = PointInRect(aimx, aimy, minusX, ctrlY, ctrlBtnS, ctrlBtnS);
				Color mFill = hMinus ? (Color){100, 60, 60, 255} : (Color){50, 30, 30, 255};
				Color mLine = hMinus ? WHITE : (Color){150, 120, 120, 255};
				float r = 8.0f / ctrlBtnS;
				if (r > 1.0f) r = 1.0f;
				DrawFilledRectRounded((float)minusX, (float)ctrlY,
				                      (float)ctrlBtnS, (float)ctrlBtnS, r, mFill);
				DrawRectRoundedLines((float)minusX, (float)ctrlY,
				                     (float)ctrlBtnS, (float)ctrlBtnS, r, mLine);
				Color mText = hMinus ? WHITE : (Color){200, 180, 180, 255};
				int mtw = MeasureTextStr("-", ctrlFontH);
				DrawTextStr("-", (float)(minusX + (ctrlBtnS - mtw)/2),
				            (float)(ctrlY + 1), ctrlFontH, mText);

				char valBuf[8];
				sprintf(valBuf, "%d", customBalls);
				int vtw = MeasureTextStr(valBuf, ctrlFontH);
				DrawTextStr(valBuf, (float)(valX + (valW - vtw)/2),
				            (float)(ctrlY + 2), ctrlFontH, (Color){255, 255, 200, 255});

				bool hPlus = PointInRect(aimx, aimy, plusX, ctrlY, ctrlBtnS, ctrlBtnS);
				Color pFill = hPlus ? (Color){60, 100, 60, 255} : (Color){30, 50, 30, 255};
				Color pLine = hPlus ? WHITE : (Color){120, 150, 120, 255};
				DrawFilledRectRounded((float)plusX, (float)ctrlY,
				                      (float)ctrlBtnS, (float)ctrlBtnS, r, pFill);
				DrawRectRoundedLines((float)plusX, (float)ctrlY,
				                     (float)ctrlBtnS, (float)ctrlBtnS, r, pLine);
				Color pText = hPlus ? WHITE : (Color){180, 200, 180, 255};
				int ptw = MeasureTextStr("+", ctrlFontH);
				DrawTextStr("+", (float)(plusX + (ctrlBtnS - ptw)/2),
				            (float)(ctrlY + 1), ctrlFontH, pText);
			}

			// ---- Row 2: Shots ----
			{
				int minusX = rowCenterX - labelW/2;
				int valX   = minusX + ctrlBtnS + 4;
				int plusX  = valX + valW + 4;
				int infX   = plusX + ctrlBtnS + 8;
				int ctrlY  = row2Y;

				DrawTextStr("Shots:", (float)(rowCenterX - labelW/2 - MeasureTextStr("Shots: ", ctrlFontH) - 10),
				            (float)(ctrlY + 2), ctrlFontH, (Color){200, 200, 200, 255});

				bool shotsActive = !customShotsInfinite;
				float r = 8.0f / ctrlBtnS;
				if (r > 1.0f) r = 1.0f;

				bool hMinus = shotsActive && PointInRect(aimx, aimy, minusX, ctrlY, ctrlBtnS, ctrlBtnS);
				Color mFill = (!shotsActive) ? (Color){25, 25, 25, 255} : (hMinus ? (Color){100, 60, 60, 255} : (Color){50, 30, 30, 255});
				Color mLine = (!shotsActive) ? (Color){60, 60, 60, 255} : (hMinus ? WHITE : (Color){150, 120, 120, 255});
				DrawFilledRectRounded((float)minusX, (float)ctrlY,
				                      (float)ctrlBtnS, (float)ctrlBtnS, r, mFill);
				DrawRectRoundedLines((float)minusX, (float)ctrlY,
				                     (float)ctrlBtnS, (float)ctrlBtnS, r, mLine);
				Color mText = (!shotsActive) ? (Color){80, 80, 80, 255} : (hMinus ? WHITE : (Color){200, 180, 180, 255});
				int mtw = MeasureTextStr("-", ctrlFontH);
				DrawTextStr("-", (float)(minusX + (ctrlBtnS - mtw)/2),
				            (float)(ctrlY + 1), ctrlFontH, mText);

				char valBuf[8];
				sprintf(valBuf, "%d", customShots);
				int vtw = MeasureTextStr(valBuf, ctrlFontH);
				DrawTextStr(valBuf, (float)(valX + (valW - vtw)/2),
				            (float)(ctrlY + 2), ctrlFontH,
				            shotsActive ? (Color){255, 255, 200, 255} : (Color){100, 100, 80, 255});

				bool hPlus = shotsActive && PointInRect(aimx, aimy, plusX, ctrlY, ctrlBtnS, ctrlBtnS);
				Color pFill = (!shotsActive) ? (Color){25, 25, 25, 255} : (hPlus ? (Color){60, 100, 60, 255} : (Color){30, 50, 30, 255});
				Color pLine = (!shotsActive) ? (Color){60, 60, 60, 255} : (hPlus ? WHITE : (Color){120, 150, 120, 255});
				DrawFilledRectRounded((float)plusX, (float)ctrlY,
				                      (float)ctrlBtnS, (float)ctrlBtnS, r, pFill);
				DrawRectRoundedLines((float)plusX, (float)ctrlY,
				                     (float)ctrlBtnS, (float)ctrlBtnS, r, pLine);
				Color pText = (!shotsActive) ? (Color){80, 80, 80, 255} : (hPlus ? WHITE : (Color){180, 200, 180, 255});
				int ptw = MeasureTextStr("+", ctrlFontH);
				DrawTextStr("+", (float)(plusX + (ctrlBtnS - ptw)/2),
				            (float)(ctrlY + 1), ctrlFontH, pText);

				bool hInf = PointInRect(aimx, aimy, infX, ctrlY, ctrlBtnS + 6, ctrlBtnS);
				Color iFill = customShotsInfinite ? (hInf ? (Color){80, 80, 140, 255} : (Color){50, 50, 100, 255}) : (hInf ? (Color){50, 50, 50, 255} : (Color){30, 30, 30, 255});
				Color iLine = customShotsInfinite ? (hInf ? WHITE : (Color){150, 150, 220, 255}) : (hInf ? (Color){180, 180, 180, 255} : (Color){100, 100, 100, 255});
				DrawFilledRectRounded((float)infX, (float)ctrlY,
				                      (float)(ctrlBtnS + 6), (float)ctrlBtnS, r, iFill);
				DrawRectRoundedLines((float)infX, (float)ctrlY,
				                     (float)(ctrlBtnS + 6), (float)ctrlBtnS, r, iLine);
				Color iText = customShotsInfinite ? (hInf ? WHITE : (Color){200, 200, 255, 255}) : (hInf ? WHITE : (Color){150, 150, 150, 255});
				int iw = MeasureTextStr("inf", ctrlFontH - 2);
				DrawTextStr("inf", (float)(infX + (ctrlBtnS + 6 - iw)/2),
				            (float)(ctrlY + 2), ctrlFontH - 2, iText);
			}

			// ---- START GAME and BACK buttons ----
			int actFontH = ctrlFontH + 2;
			int startBtnW = winWidth * 30 / 100;
			if (startBtnW < 120) startBtnW = 120;
			int startBtnH = winHeight * 8 / 100;
			if (startBtnH < 32) startBtnH = 32;
			int startBtnX = centerX - startBtnW / 2;
			int startBtnY = winHeight * 65 / 100;
			int backBtnY = startBtnY + startBtnH + 8;

			bool hoverStart = PointInRect(aimx, aimy, startBtnX, startBtnY, startBtnW, startBtnH);
			bool hoverBack  = PointInRect(aimx, aimy, startBtnX, backBtnY, startBtnW, startBtnH);

			drawButton(startBtnX, startBtnY, startBtnW, startBtnH, "START GAME", actFontH, hoverStart);
			drawButton(startBtnX, backBtnY,  startBtnW, startBtnH, "BACK", actFontH, hoverBack);
			break;
		}

		// ==================== PLAYING ====================
		case PLAYING:
		{
			drawSpiralGuide();
			int id;
			bool collision = collisionDetection(head, cball, &id);
			if (collision)
			{
				ListInsert(head, id, cball);
				updateBallPos(head);
				int earned = EliminateRuns(head);
				totalScore += earned;
				if (earned > 0 && popupCount < MAX_POPUPS) {
					ScorePopup sp;
					sp.x = cball.x;
					sp.y = cball.y;
					sp.score = earned;
					sp.life = 30;
					popups[popupCount++] = sp;
				}
				updateBallPos(head);

				if (head->next == NULL) {
					gameWon = true;
					state = CLEARING;
					clearFrame = 0;
				}

				cball.c = rand() % 6;
				drawColBall(&cball, (float)centerX, (float)centerY);
				ballMoving = false;

				if (state == PLAYING && remainingShots == 0 && head->next != NULL) {
					int remBalls = 0;
					Node* np = head->next;
					while (np != NULL) { remBalls++; np = np->next; }
					totalScore -= remBalls * 20;
					if (totalScore < 0) totalScore = 0;
					gameWon = false;
					state = CLEARING;
					clearFrame = 0;
				}
			}
			drawBallList(head);

			// Boundary check
			if (cball.x > winWidth || cball.x < 0 || cball.y > winHeight || cball.y < 0)
			{
				cball.c = rand() % 6;
				drawColBall(&cball, (float)centerX, (float)centerY);
				ballMoving = false;

				if (remainingShots == 0 && head->next != NULL) {
					int remBalls = 0;
					Node* np = head->next;
					while (np != NULL) { remBalls++; np = np->next; }
					totalScore -= remBalls * 20;
					if (totalScore < 0) totalScore = 0;
					gameWon = false;
					state = CLEARING;
					clearFrame = 0;
				}
			}
			if (!ballMoving && aiming) {
				DrawWideLine(cball.x, cball.y, (float)aimx, (float)aimy, WHITE);
			}

			// Move / render collision ball
			if (ballMoving) {
				drawColBall(&cball, cball.x += vx, cball.y -= vy);
			} else {
				drawColBall(&cball, (float)centerX, (float)centerY);
			}
			break;
		}

		// ==================== CLEARING ====================
		case CLEARING:
		{
			clearFrame++;

			if (clearFrame < 60) {
				drawSpiralGuide();
				drawColBall(&cball, (float)centerX, (float)centerY);

				int startGap = winHeight / 37;
				if (startGap < 2) startGap = 2;
				int lineGap = startGap - clearFrame * (startGap - 1) / 59;
				if (lineGap < 1) lineGap = 1;
				for (int y = 0; y < winHeight; y += lineGap)
					DrawWideLine(0.0f, (float)y, (float)winWidth, (float)y, BLACK);

			} else if (clearFrame < 120) {
				float t = (clearFrame - 60) / 60.0f;
				if (t > 1.0f) t = 1.0f;

				int cornerFontH = BALLRADIUS * 3;
				if (cornerFontH < 14) cornerFontH = 14;
				int centerFontH = winHeight / 6;
				if (centerFontH < 24) centerFontH = 24;
				int curFontH = cornerFontH + (int)((centerFontH - cornerFontH) * t);

				char scoreStr[32];
				sprintf(scoreStr, "Score: %d", totalScore);
				int tw = MeasureTextStr(scoreStr, curFontH);

				int startX = winWidth - tw - 10;
				int startY = 10;
				int endX = centerX - tw / 2;
				int endY = centerY - curFontH / 2;

				int curX = startX + (int)((endX - startX) * t);
				int curY = startY + (int)((endY - startY) * t);
				DrawTextStr(scoreStr, (float)curX, (float)curY,
				            curFontH, (Color){255, 255, 200, 255});
			} else {
				state = SETTLEMENT;
			}

			// Popups and corner score during Phase A
			if (clearFrame < 60) {
				for (int i = 0; i < popupCount; ) {
					popups[i].y -= BALLRADIUS * 0.06f;
					popups[i].life--;
					if (popups[i].life <= 0) {
						popups[i] = popups[--popupCount];
					} else {
						i++;
					}
				}
				int popupFontH = BALLRADIUS * 2;
				if (popupFontH < 10) popupFontH = 10;
				for (int i = 0; i < popupCount; i++) {
					int brightness = popups[i].life * 255 / 30;
					char buf[16];
					sprintf(buf, "+%d", popups[i].score);
					int tw = MeasureTextStr(buf, popupFontH);
					DrawTextStr(buf, popups[i].x - (float)tw / 2, popups[i].y,
					            popupFontH, (Color){(unsigned char)brightness, (unsigned char)brightness, 0, 255});
				}

				int scoreFontH = BALLRADIUS * 3;
				if (scoreFontH < 14) scoreFontH = 14;
				char scoreStr[32];
				sprintf(scoreStr, "Score: %d", totalScore);
				int sw = MeasureTextStr(scoreStr, scoreFontH);
				DrawTextStr(scoreStr, (float)(winWidth - sw - 10), 10.0f,
				            scoreFontH, (Color){255, 255, 200, 255});
			}
			break;
		}

		// ==================== SETTLEMENT ====================
		case SETTLEMENT:
		{
			int titleFontH = winHeight / 12;
			if (titleFontH < 18) titleFontH = 18;
			if (gameWon) {
				const char* ttl = "CLEAR!";
				int ttw = MeasureTextStr(ttl, titleFontH);
				DrawTextStr(ttl, (float)(centerX - ttw / 2),
				            (float)(winHeight / 8), titleFontH,
				            (Color){100, 255, 100, 255});
			} else {
				const char* ttl = "GAME OVER";
				int ttw = MeasureTextStr(ttl, titleFontH);
				DrawTextStr(ttl, (float)(centerX - ttw / 2),
				            (float)(winHeight / 8), titleFontH,
				            (Color){255, 100, 100, 255});
			}

			int scoreFontH = winHeight / 6;
			if (scoreFontH < 24) scoreFontH = 24;
			char scoreStr[32];
			sprintf(scoreStr, "Score: %d", totalScore);
			int sw = MeasureTextStr(scoreStr, scoreFontH);
			DrawTextStr(scoreStr, (float)(centerX - sw / 2),
			            (float)(centerY - scoreFontH), scoreFontH,
			            (Color){255, 255, 200, 255});

			int btnW = winWidth * 3 / 10;
			if (btnW < 120) btnW = 120;
			int btnH = winHeight * 8 / 100;
			if (btnH < 36) btnH = 36;
			int btnFontH = btnH / 2;
			int btnSpacing = btnH / 2;
			int btnX = centerX - btnW / 2;
			int tryAgainY = centerY + winHeight / 6;
			int exitY = tryAgainY + btnH + btnSpacing;

			bool hoverTryAgain = PointInRect(aimx, aimy, btnX, tryAgainY, btnW, btnH);
			bool hoverExit     = PointInRect(aimx, aimy, btnX, exitY, btnW, btnH);
			drawButton(btnX, tryAgainY, btnW, btnH, "TRY AGAIN", btnFontH, hoverTryAgain);
			drawButton(btnX, exitY,     btnW, btnH, "EXIT",      btnFontH, hoverExit);
			break;
		}
		}

		// ---- Post-render: popups + score HUD (PLAYING only) ----
		if (state == PLAYING) {
			for (int i = 0; i < popupCount; ) {
				popups[i].y -= BALLRADIUS * 0.06f;
				popups[i].life--;
				if (popups[i].life <= 0) {
					popups[i] = popups[--popupCount];
				} else {
					i++;
				}
			}
			int popupFontH = BALLRADIUS * 2;
			if (popupFontH < 10) popupFontH = 10;
			for (int i = 0; i < popupCount; i++) {
				int brightness = popups[i].life * 255 / 30;
				char buf[16];
				sprintf(buf, "+%d", popups[i].score);
				int tw = MeasureTextStr(buf, popupFontH);
				DrawTextStr(buf, popups[i].x - (float)tw / 2, popups[i].y,
				            popupFontH, (Color){(unsigned char)brightness, (unsigned char)brightness, 0, 255});
			}
			// Draw total score in top-right corner
			int scoreFontH = BALLRADIUS * 3;
			if (scoreFontH < 14) scoreFontH = 14;
			char scoreStr[32];
			sprintf(scoreStr, "Score: %d", totalScore);
			int sw = MeasureTextStr(scoreStr, scoreFontH);
			DrawTextStr(scoreStr, (float)(winWidth - sw - 10), 10.0f,
			            scoreFontH, (Color){255, 255, 200, 255});

			// Draw remaining shots
			int shotFontH = BALLRADIUS * 2;
			if (shotFontH < 10) shotFontH = 10;
			char shotStr[32];
			Color shotColor;
			if (remainingShots < 0) {
				shotColor = (Color){150, 255, 150, 255};
				sprintf(shotStr, "Shots: inf");
			} else if (remainingShots <= 5) {
				shotColor = (Color){255, 100, 100, 255};
				sprintf(shotStr, "Shots: %d", remainingShots);
			} else {
				shotColor = (Color){200, 200, 200, 255};
				sprintf(shotStr, "Shots: %d", remainingShots);
			}
			int ssw = MeasureTextStr(shotStr, shotFontH);
			DrawTextStr(shotStr, (float)(winWidth - ssw - 10),
			            10.0f + scoreFontH + 2, shotFontH, shotColor);
		}

		EndRender();
	}

	// Cleanup on window close (X button)
	if (head != NULL)
		DestroyList(head);
	RenderClose();
	return 0;
}
