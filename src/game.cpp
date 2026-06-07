#include <time.h>
#include <math.h>
#include <stdlib.h>
#include "game.h"
#include "render.h"

#define TOTAL_THETA (4 * PI)

// ── Global (derived constant recomputed on resize) ────
int BALLRADIUS;

// ═══════════════════════════════════════════════════════
//  Init & utilities
// ═══════════════════════════════════════════════════════

void GameInit(GameContext* g)
{
	g->winWidth  = 600;
	g->winHeight = 600;
	g->centerX   = 0;
	g->centerY   = 0;
	g->R_OUTER   = 0.0;
	g->R_INNER   = 0.0;

	g->state     = MENU;
	g->gameWon   = false;
	g->clearTimer = 0.0f;
	g->clearPhase   = 0;
	g->clearBallProgress = 0.0f;
	g->clearActiveTheta  = 0.0f;

	g->startBalls = 10;
	g->startShots = -1;

	g->customBalls        = 20;
	g->customShots        = 80;
	g->customShotsInfinite = false;
	g->customHoldBtn      = -1;
	g->customHoldTime     = 0.0f;
	g->customBatchCount   = 0;

	g->head     = NULL;
	g->cball    = {0, 0, 0.0f, 0.0f, (BallColor)0};
	g->ballMoving = false;
	g->aiming     = false;
	g->aimx = 0;  g->aimy = 0;
	g->vx  = 0;   g->vy  = 0;
	g->counter      = 0;
	g->totalScore   = 0;
	g->popupCount   = 0;
	g->remainingShots = -1;

	srand((unsigned int)time(NULL));
}

void recomputeDimensions(GameContext* g)
{
	int ref = (g->winWidth < g->winHeight ? g->winWidth : g->winHeight) / 2;
	BALLRADIUS = ref * 10 / 300;
	if (BALLRADIUS < 4) BALLRADIUS = 4;
	g->R_OUTER = ref * 260.0 / 300.0;
	g->R_INNER = ref * 30.0 / 300.0;
	g->centerX = g->winWidth / 2;
	g->centerY = g->winHeight / 2;
}

void HandleResize(GameContext* g)
{
	int newW = GetRenderW();
	int newH = GetRenderH();
	if (newW < 200 || newH < 200) return;
	if (newW == g->winWidth && newH == g->winHeight) return;

	int oldCX = g->centerX, oldCY = g->centerY, oldBR = BALLRADIUS;
	g->winWidth  = newW;
	g->winHeight = newH;
	recomputeDimensions(g);

	if (g->state == PLAYING && g->head != NULL)
		updateBallPos(g, g->head);

	if (g->state == PLAYING && g->ballMoving) {
		float scale = (float)BALLRADIUS / (float)oldBR;
		g->cball.x = g->centerX + (g->cball.x - oldCX) * scale;
		g->cball.y = g->centerY + (g->cball.y - oldCY) * scale;
		g->vx *= scale;
		g->vy *= scale;
	}
	g->aimx = g->centerX + BALLRADIUS * 5;
	g->aimy = g->centerY;
}

// ═══════════════════════════════════════════════════════
//  Ball helpers
// ═══════════════════════════════════════════════════════

void initBallList(Node* head, int count)
{
	ball b;
	for (int i = 0; i < count; ++i) {
		b.c = (BallColor)(i % COLOR_COUNT);
			b.animTimer = 0.0f;
		ListInsert(head, 0, b);
	}
}

void updateBallPos(const GameContext* g, Node* head)
{
	const double k = (g->R_OUTER - g->R_INNER) / TOTAL_THETA;
	const double step = 2.0 * BALLRADIUS;
	const double dtheta = 0.002;

	double theta = 0.0;
	Node* p = head;

	while (p->next != NULL) {
		p = p->next;
		double r = g->R_OUTER - k * theta;
		p->data.x = (float)(g->centerX + r * cos(theta));
		p->data.y = (float)(g->centerY + r * sin(theta));
		p->data.theta = (float)theta;

		double arc = 0.0;
		while (arc < step) {
			double r_cur = g->R_OUTER - k * theta;
			double r_next = g->R_OUTER - k * (theta + dtheta);
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

void drawBallList(Node* head)
{
	Node* p = head;
	while (p->next != NULL) {
		p = p->next;
		DrawFilledCircle(p->data.x, p->data.y, (float)BALLRADIUS,
		                 ballColorTable[(int)p->data.c]);
	}
}

void drawColBall(ball* b, float x, float y)
{
	b->x = x;
	b->y = y;
	DrawFilledCircle(b->x, b->y, (float)BALLRADIUS,
	                 ballColorTable[(int)b->c]);
}

// ═══════════════════════════════════════════════════════
//  Rendering helpers
// ═══════════════════════════════════════════════════════

void drawSpiralGuide(const GameContext* g)
{
	const double k = (g->R_OUTER - g->R_INNER) / TOTAL_THETA;
	const double dtheta = 0.02;
	const double dashLen = BALLRADIUS * 1.4;
	const double gapLen  = BALLRADIUS * 0.8;

	Color guideColor = {160, 160, 160, 255};

	double theta = 0.0;
	double r = g->R_OUTER;
	float prev_x = (float)(g->centerX + r * cos(theta));
	float prev_y = (float)(g->centerY + r * sin(theta));

	double segArc = 0.0;
	bool drawing = true;

	while (theta <= TOTAL_THETA + 0.5) {
		theta += dtheta;
		r = g->R_OUTER - k * theta;
		if (r < g->R_INNER) break;
		float x = (float)(g->centerX + r * cos(theta));
		float y = (float)(g->centerY + r * sin(theta));

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

void drawButton(int x, int y, int w, int h,
                const char* text, int fontH, bool hovered)
{
	Color fillColor = hovered ? (Color){80, 80, 80, 255}
	                          : (Color){40, 40, 40, 255};
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

// ═══════════════════════════════════════════════════════
//  State helpers (shared by input + render)
// ═══════════════════════════════════════════════════════

static void startGame(GameContext* g)
{
	g->head = CreateEmptyList();
	initBallList(g->head, g->startBalls);
	updateBallPos(g, g->head);
	g->cball.c = (BallColor)(rand() % COLOR_COUNT);
	g->cball.x = (float)g->centerX;
	g->cball.y = (float)g->centerY;
	g->ballMoving = false;
	g->aiming     = false;
	g->totalScore = 0;
	g->popupCount = 0;
	g->remainingShots = g->startShots;
	g->gameWon = false;
	g->state = PLAYING;
}

static void finishGame(GameContext* g, bool won)
{
	g->gameWon = won;
	g->state = CLEARING;
	g->clearTimer = 0.0f;
	g->clearPhase = 0;
	g->clearBallProgress = 0.0f;
	g->clearActiveTheta  = 0.0f;
}

static void resetBall(GameContext* g)
{
	g->cball.c = (BallColor)(rand() % COLOR_COUNT);
	g->cball.x = (float)g->centerX;
	g->cball.y = (float)g->centerY;
	g->ballMoving = false;
}

// ═══════════════════════════════════════════════════════
//  Input handlers
// ═══════════════════════════════════════════════════════

void InputMenu(GameContext* g)
{
	Vector2 mp = GetInputMousePos();
	int mx = (int)mp.x, my = (int)mp.y;
	g->aimx = mx; g->aimy = my;

	if (IsInputLeftReleased()) {
		int btnW = g->winWidth * 55 / 100;
		if (btnW < 200) btnW = 200;
		int btnH = g->winHeight * 10 / 100;
		if (btnH < 40) btnH = 40;
		int gap = btnH + 6;
		int startY = g->winHeight * 3 / 10;
		int btnX = g->centerX - btnW / 2;

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
					g->state = CUSTOMIZE;
				} else {
					g->startBalls = diffs[i].balls;
					g->startShots = diffs[i].shots;
					startGame(g);
				}
				break;
			}
		}
		// ── EXIT 按钮 ──
		int exitBtnW = btnW * 3 / 5;
		int exitBtnH = btnH * 3 / 4;
		int exitBtnY = startY + 5 * gap + btnH / 2;
		int exitBtnX = g->centerX - exitBtnW / 2;
		if (PointInRect(mx, my, exitBtnX, exitBtnY, exitBtnW, exitBtnH)) {
			RenderClose();
			exit(0);
		}
	}
}
void InputCustomize(GameContext* g)
{
	Vector2 mp = GetInputMousePos();
	int mx = (int)mp.x, my = (int)mp.y;
	g->aimx = mx; g->aimy = my;

	// ── Compute button layout ──────────────────────────
	int ctrlBtnS = g->winHeight / 20;
	if (ctrlBtnS < 22) ctrlBtnS = 22;
	if (ctrlBtnS > 36) ctrlBtnS = 36;
	int ctrlFontH = ctrlBtnS * 3 / 4;
	int rowH = ctrlBtnS + 8;
	int row1Y = g->winHeight * 35 / 100;
	int row2Y = row1Y + rowH + 12;

	int labelW = MeasureTextStr("Shots:  ", ctrlFontH) + 10;
	int valW = MeasureTextStr("200", ctrlFontH) + 30;

	int ballsMinusX = g->centerX - labelW/2;
	int ballsPlusX  = ballsMinusX + ctrlBtnS + 4 + valW + 4;
	int shotsMinusX = g->centerX - labelW/2;
	int shotsPlusX  = shotsMinusX + ctrlBtnS + 4 + valW + 4;
	int shotsInfX   = shotsPlusX + ctrlBtnS + 8;

	int startBtnW = g->winWidth * 30 / 100;
	if (startBtnW < 120) startBtnW = 120;
	int startBtnH = g->winHeight * 8 / 100;
	if (startBtnH < 32) startBtnH = 32;
	int startBtnX = g->centerX - startBtnW / 2;
	int startBtnY = g->winHeight * 65 / 100;
	int backBtnY  = startBtnY + startBtnH + 8;

	// ── Determine hovered +/- button ───────────────────
	// 0=balls-, 1=balls+, 2=shots-, 3=shots+
	int hoveredBtn = -1;
	if (PointInRect(mx, my, ballsMinusX, row1Y, ctrlBtnS, ctrlBtnS))
		hoveredBtn = 0;
	else if (PointInRect(mx, my, ballsPlusX, row1Y, ctrlBtnS, ctrlBtnS))
		hoveredBtn = 1;
	else if (!g->customShotsInfinite &&
	         PointInRect(mx, my, shotsMinusX, row2Y, ctrlBtnS, ctrlBtnS))
		hoveredBtn = 2;
	else if (!g->customShotsInfinite &&
	         PointInRect(mx, my, shotsPlusX, row2Y, ctrlBtnS, ctrlBtnS))
		hoveredBtn = 3;

	float dt = GetFrameTime();
	bool leftDown = IsInputLeftDown();
	bool leftReleased = IsInputLeftReleased();

	// ── Hold tracking ──────────────────────────────────
	if (leftDown && hoveredBtn >= 0) {
		if (g->customHoldBtn == hoveredBtn) {
			g->customHoldTime += dt;
		} else if (g->customHoldBtn == -1) {
			g->customHoldBtn = hoveredBtn;
			g->customHoldTime = 0.0f;
			g->customBatchCount = 0;
		}
	} else if (!leftDown && !leftReleased) {
		g->customHoldBtn = -1;
		g->customHoldTime = 0.0f;
		g->customBatchCount = 0;
	}

	// ── Batch mode: hold >1s, fire every 0.3s (+/-10) ─
	if (g->customHoldBtn >= 0 && g->customHoldTime >= 0.6f) {
		int batchIdx = (int)((g->customHoldTime - 0.6f) / 0.3f);
		if (batchIdx > g->customBatchCount) {
			int repeat = batchIdx - g->customBatchCount;
			int delta = repeat * 10;
			switch (g->customHoldBtn) {
			case 0: g->customBalls = (g->customBalls - delta >= 5)
			        ? g->customBalls - delta : 5; break;
			case 1: g->customBalls = (g->customBalls + delta <= 50)
			        ? g->customBalls + delta : 50; break;
			case 2: g->customShots = (g->customShots - delta >= 10)
			        ? g->customShots - delta : 10; break;
			case 3: g->customShots = (g->customShots + delta <= 200)
			        ? g->customShots + delta : 200; break;
			}
			g->customBatchCount = batchIdx;
		}
	}

	// ── Release / single-click ─────────────────────────
	if (leftReleased) {
		if (g->customHoldBtn >= 0 && g->customHoldTime < 0.6f) {
			// Single click on +/- button
			switch (g->customHoldBtn) {
			case 0: if (g->customBalls > 5) g->customBalls--; break;
			case 1: if (g->customBalls < 50) g->customBalls++; break;
			case 2: if (g->customShots > 10) g->customShots--; break;
			case 3: if (g->customShots < 200) g->customShots++; break;
			}
		} else if (g->customHoldBtn == -1) {
			// Click on non-hold buttons
			if (PointInRect(mx, my, shotsInfX, row2Y,
			                ctrlBtnS + 6, ctrlBtnS)) {
				g->customShotsInfinite = !g->customShotsInfinite;
			}
			else if (PointInRect(mx, my, startBtnX, startBtnY,
			                     startBtnW, startBtnH)) {
				g->startBalls = g->customBalls;
				g->startShots = g->customShotsInfinite ? -1 : g->customShots;
				startGame(g);
			}
			else if (PointInRect(mx, my, startBtnX, backBtnY,
			                     startBtnW, startBtnH)) {
				g->state = MENU;
			}
		}
		g->customHoldBtn = -1;
		g->customHoldTime = 0.0f;
		g->customBatchCount = 0;
	}
}

void InputPlaying(GameContext* g)
{
	Vector2 mp = GetInputMousePos();
	int mx = (int)mp.x, my = (int)mp.y;
	g->aimx = mx; g->aimy = my;

	// ── EXIT 按钮 ──
	int exitW = BALLRADIUS * 6;
	if (exitW < 60) exitW = 60;
	int exitH = BALLRADIUS * 5 / 2;
	if (exitH < 24) exitH = 24;
	int exitX = 10, exitY = 10;
	if (IsInputLeftReleased() && PointInRect(mx, my, exitX, exitY, exitW, exitH)) {
		finishGame(g, false);
		return;
	}

	if (!g->ballMoving) {
		if (IsInputLeftDown())
			g->aiming = true;
		if (IsInputLeftReleased() && g->aiming) {
			float dx = (float)(mx - g->centerX);
			float dy = (float)(g->centerY - my);
			float length = sqrtf(dx * dx + dy * dy);
			if (length > 0) {
				g->vx = (dx / length) * BALLRADIUS;
				g->vy = (dy / length) * BALLRADIUS;
			}
			if (g->remainingShots > 0)
				g->remainingShots--;
			g->ballMoving = true;
			g->aiming = false;
		}
	}
}

void InputSettlement(GameContext* g)
{
	Vector2 mp = GetInputMousePos();
	int mx = (int)mp.x, my = (int)mp.y;
	g->aimx = mx; g->aimy = my;

	if (IsInputLeftReleased()) {
		int btnW = g->winWidth * 3 / 10;
		if (btnW < 120) btnW = 120;
		int btnH = g->winHeight * 8 / 100;
		if (btnH < 36) btnH = 36;
		int btnSpacing = btnH / 2;
		int btnX = g->centerX - btnW / 2;
		int tryAgainY = g->centerY + g->winHeight / 6;
		int exitY = tryAgainY + btnH + btnSpacing;

		if (PointInRect(mx, my, btnX, tryAgainY, btnW, btnH)) {
			DestroyList(g->head);
			g->head = NULL;
			g->state = MENU;
		}
		else if (PointInRect(mx, my, btnX, exitY, btnW, btnH)) {
			DestroyList(g->head);
			g->head = NULL;
			RenderClose();
			exit(0);
		}
	}
}

// ═══════════════════════════════════════════════════════
//  Game update (PLAYING only)
// ═══════════════════════════════════════════════════════

void UpdatePlaying(GameContext* g)
{
	if (!g->ballMoving) return;

	// Move ball
	g->cball.x += g->vx;
	g->cball.y -= g->vy;

	// Boundary check
	if (g->cball.x > g->winWidth || g->cball.x < 0 ||
	    g->cball.y > g->winHeight || g->cball.y < 0) {
		resetBall(g);
		if (g->remainingShots == 0 && g->head->next != NULL)
			finishGame(g, false);
		return;
	}

	// Collision check
	int id;
	if (!collisionDetection(g->head, g->cball, &id)) return;

	ListInsert(g->head, id, g->cball);
	updateBallPos(g, g->head);

	int earned = EliminateRuns(g->head);
	g->totalScore += earned;
	if (earned > 0 && g->popupCount < MAX_POPUPS) {
		ScorePopup sp;
		sp.x = g->cball.x;
		sp.y = g->cball.y;
		sp.score = earned;
		sp.life = 30;
		g->popups[g->popupCount++] = sp;
	}
	updateBallPos(g, g->head);

	if (g->head->next == NULL) {
		finishGame(g, true);
		return;
	}

	resetBall(g);

	if (g->remainingShots == 0 && g->head->next != NULL)
		finishGame(g, false);
}

// ═══════════════════════════════════════════════════════
//  Game update (CLEARING only)
// ═══════════════════════════════════════════════════════

// ── 缓动函数：加速(25%) → 匀速(50%) → 减速(25%) ──
static float easeBallProgress(float t)
{
	if (t < 0.25f) {
		float u = t / 0.25f;
		return 0.164f * u * u;
	} else if (t < 0.75f) {
		return 0.164f + 1.312f * (t - 0.25f);
	} else {
		float u = 1.0f - t;
		return 1.0f - 0.131f * u - 2.362f * u * u;
	}
}

// ── 球到中心的螺旋弧长近似 ──
static float spiralDistToCenter(const GameContext* g, const ball* b)
{
	const double k = (g->R_OUTER - g->R_INNER) / TOTAL_THETA;
	double thetaMax = g->R_OUTER / k;
	double dTheta = thetaMax - (double)b->theta;
	if (dTheta < 0.0) dTheta = 0.0;
	double r = g->R_OUTER - k * (double)b->theta;
	if (r < 0.0) r = 0.0;
	return (float)((r * 0.55 + (double)BALLRADIUS * 3.0) * dTheta);
}

void UpdateClearing(GameContext* g)
{
	float dt = GetFrameTime();
	g->clearTimer += dt;

	// ── Phase 0: 球链沿螺旋线依次收缩消除 ──
	if (g->clearPhase == 0) {
		// 无剩余球或已通关 → 跳过 Phase 0
		if (g->head->next == NULL || g->gameWon) {
			g->clearPhase = 1;
			g->clearTimer = 0.0f;
			return;
		}
		// 从尾部（最内层）向前推进动画
		// 找到最内层球（链表尾部）
		// 从尾部（最内层）向前推进动画
		Node* tail = g->head;
		while (tail->next != NULL) tail = tail->next;

		// 尾部球动画
		float totalT = spiralDistToCenter(g, &tail->data)
		    / (1.5f * (float)BALLRADIUS * 60.0f);
		if (totalT < 0.15f) totalT = 0.15f;
	// 确保动画计时器从0开始（首次进入Phase 0）

		tail->data.animTimer += dt;
		float tTail = tail->data.animTimer / totalT;
		if (tTail > 1.0f) tTail = 1.0f;

		// 尾部球到达中央 → 移除并扣分
		if (tTail >= 1.0f) {
			if (tail->prev != NULL)
				tail->prev->next = NULL;
			g->totalScore -= 20;
			if (g->totalScore < 0) g->totalScore = 0;
			if (g->popupCount < MAX_POPUPS) {
				ScorePopup sp;
				sp.x = (float)g->centerX;
				sp.y = (float)g->centerY;
				sp.score = -20;
				sp.life = 30;
				g->popups[g->popupCount++] = sp;
			}
			free(tail);
		}

		// 重新找尾部并向前激活外层球
		if (g->head->next != NULL) {
			Node* t = g->head;
			while (t->next != NULL) t = t->next;
			float tt = spiralDistToCenter(g, &t->data)
			    / (1.5f * (float)BALLRADIUS * 60.0f);
			if (tt < 0.15f) tt = 0.15f;
			float tn = t->data.animTimer / tt;
			if (tn > 1.0f) tn = 1.0f;
			bool activate = (tn >= 0.25f);

			Node* b = t->prev;
			while (b != g->head && activate) {
				float bt = spiralDistToCenter(g, &b->data)
				    / (1.5f * (float)BALLRADIUS * 60.0f);
				if (bt < 0.15f) bt = 0.15f;
				b->data.animTimer += dt;
				float bn = b->data.animTimer / bt;
				if (bn > 1.0f) bn = 1.0f;
				activate = (bn >= 0.25f);
				b = b->prev;
			}
		}

		// 全部移除 → Phase 1
		if (g->head->next == NULL) {
			g->clearPhase = 1;
			g->clearTimer = 0.0f;
		}
return;

	}

	// ── Phase 1: 百叶窗淡出 ──
	if (g->clearPhase == 1) {
		if (g->clearTimer > 0.8f) {
			g->clearPhase = 2;
			g->clearTimer = 0.0f;
		}
		return;
	}

	// ── Phase 2: 得分飞入中央 → SETTLEMENT ──
	if (g->clearPhase == 2) {
		if (g->clearTimer > 1.0f) {
			g->state = SETTLEMENT;
		}
		return;
	}
}

// ═══════════════════════════════════════════════════════
//  Render handlers
// ═══════════════════════════════════════════════════════

void RenderMenu(const GameContext* g)
{
	int titleFontH = g->winHeight / 8;
	if (titleFontH < 24) titleFontH = 24;
	DrawTextStr("ZUMA",
	            (float)(g->centerX - MeasureTextStr("ZUMA", titleFontH) / 2),
	            (float)(g->winHeight / 12),
	            titleFontH, (Color){255, 200, 50, 255});

	struct { const char* name; const char* desc; } diffs[] = {
		{"INFINITE",   "Balls: 10   |  Shots: Infinite"},
		{"EASY",       "Balls: 10   |  Shots: 100"},
		{"COMMON",     "Balls: 20   |  Shots: 80"},
		{"HARD",       "Balls: 30   |  Shots: 50"},
		{"CUSTOMIZED", "Set your own rules"},
	};

	int btnW = g->winWidth * 55 / 100;
	if (btnW < 200) btnW = 200;
	int btnH = g->winHeight * 10 / 100;
	if (btnH < 40) btnH = 40;
	int gap = btnH + 6;
	int startY = g->winHeight * 3 / 10;

	for (int i = 0; i < 5; i++) {
		int btnX = g->centerX - btnW / 2;
		int btnY = startY + i * gap;
		bool hover = PointInRect(g->aimx, g->aimy, btnX, btnY, btnW, btnH);

		Color btnFill = hover ? (Color){60, 60, 80, 255}
		                      : (Color){30, 30, 50, 255};
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
		Color descColor = hover ? (Color){200, 200, 255, 255}
		                        : (Color){140, 140, 180, 255};
		int dtw = MeasureTextStr(diffs[i].desc, descFontH);
		DrawTextStr(diffs[i].desc,
		            (float)(btnX + (btnW - dtw) / 2),
		            (float)(btnY + btnH * 62 / 100),
		            descFontH, descColor);
	}

	// ── EXIT 按钮 ──
	int exitBtnW = btnW * 3 / 5;
	int exitBtnH = btnH * 3 / 4;
	int exitBtnY = startY + 5 * gap + btnH / 2;
	int exitBtnX = g->centerX - exitBtnW / 2;
	bool hoverExit = PointInRect(g->aimx, g->aimy, exitBtnX, exitBtnY, exitBtnW, exitBtnH);
	int exitFontH = exitBtnH * 55 / 100;
	drawButton(exitBtnX, exitBtnY, exitBtnW, exitBtnH, "EXIT", exitFontH, hoverExit);
}

void RenderCustomize(const GameContext* g)
{
	int titleFontH = g->winHeight / 10;
	if (titleFontH < 20) titleFontH = 20;
	const char* title = "CUSTOMIZED";
	int ttw = MeasureTextStr(title, titleFontH);
	DrawTextStr(title, (float)(g->centerX - ttw / 2),
	            (float)(g->winHeight / 12), titleFontH,
	            (Color){200, 200, 255, 255});

	int ctrlBtnS = g->winHeight / 20;
	if (ctrlBtnS < 22) ctrlBtnS = 22;
	if (ctrlBtnS > 36) ctrlBtnS = 36;
	int ctrlFontH = ctrlBtnS * 3 / 4;
	int rowH = ctrlBtnS + 8;
	int row1Y = g->winHeight * 35 / 100;
	int row2Y = row1Y + rowH + 12;

	int labelW = MeasureTextStr("Shots:  ", ctrlFontH) + 10;
	int valW   = MeasureTextStr("200", ctrlFontH) + 30;

	// ── Row 1: Balls ──
	{
		int minusX = g->centerX - labelW/2;
		int valX   = minusX + ctrlBtnS + 4;
		int plusX  = valX + valW + 4;
		int ctrlY  = row1Y;

		DrawTextStr("Balls:",
		            (float)(g->centerX - labelW/2 -
		             MeasureTextStr("Balls: ", ctrlFontH) - 10),
		            (float)(ctrlY + 2), ctrlFontH,
		            (Color){200, 200, 200, 255});

		bool hMinus = PointInRect(g->aimx, g->aimy, minusX, ctrlY, ctrlBtnS, ctrlBtnS);
		Color mFill = hMinus ? (Color){100, 60, 60, 255}
		                     : (Color){50, 30, 30, 255};
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
		sprintf(valBuf, "%d", g->customBalls);
		int vtw = MeasureTextStr(valBuf, ctrlFontH);
		DrawTextStr(valBuf, (float)(valX + (valW - vtw)/2),
		            (float)(ctrlY + 2), ctrlFontH,
		            (Color){255, 255, 200, 255});

		bool hPlus = PointInRect(g->aimx, g->aimy, plusX, ctrlY, ctrlBtnS, ctrlBtnS);
		Color pFill = hPlus ? (Color){60, 100, 60, 255}
		                    : (Color){30, 50, 30, 255};
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

	// ── Row 2: Shots ──
	{
		int minusX = g->centerX - labelW/2;
		int valX   = minusX + ctrlBtnS + 4;
		int plusX  = valX + valW + 4;
		int infX   = plusX + ctrlBtnS + 8;
		int ctrlY  = row2Y;

		DrawTextStr("Shots:",
		            (float)(g->centerX - labelW/2 -
		             MeasureTextStr("Shots: ", ctrlFontH) - 10),
		            (float)(ctrlY + 2), ctrlFontH,
		            (Color){200, 200, 200, 255});

		bool shotsActive = !g->customShotsInfinite;
		float r = 8.0f / ctrlBtnS;
		if (r > 1.0f) r = 1.0f;

		bool hMinus = shotsActive && PointInRect(g->aimx, g->aimy, minusX, ctrlY, ctrlBtnS, ctrlBtnS);
		Color mFill = (!shotsActive) ? (Color){25, 25, 25, 255}
		             : (hMinus ? (Color){100, 60, 60, 255} : (Color){50, 30, 30, 255});
		Color mLine = (!shotsActive) ? (Color){60, 60, 60, 255}
		             : (hMinus ? WHITE : (Color){150, 120, 120, 255});
		DrawFilledRectRounded((float)minusX, (float)ctrlY,
		                      (float)ctrlBtnS, (float)ctrlBtnS, r, mFill);
		DrawRectRoundedLines((float)minusX, (float)ctrlY,
		                     (float)ctrlBtnS, (float)ctrlBtnS, r, mLine);
		Color mText = (!shotsActive) ? (Color){80, 80, 80, 255}
		             : (hMinus ? WHITE : (Color){200, 180, 180, 255});
		int mtw = MeasureTextStr("-", ctrlFontH);
		DrawTextStr("-", (float)(minusX + (ctrlBtnS - mtw)/2),
		            (float)(ctrlY + 1), ctrlFontH, mText);

		char valBuf[8];
		sprintf(valBuf, "%d", g->customShots);
		int vtw = MeasureTextStr(valBuf, ctrlFontH);
		DrawTextStr(valBuf, (float)(valX + (valW - vtw)/2),
		            (float)(ctrlY + 2), ctrlFontH,
		            shotsActive ? (Color){255, 255, 200, 255}
		                        : (Color){100, 100, 80, 255});

		bool hPlus = shotsActive && PointInRect(g->aimx, g->aimy, plusX, ctrlY, ctrlBtnS, ctrlBtnS);
		Color pFill = (!shotsActive) ? (Color){25, 25, 25, 255}
		             : (hPlus ? (Color){60, 100, 60, 255} : (Color){30, 50, 30, 255});
		Color pLine = (!shotsActive) ? (Color){60, 60, 60, 255}
		             : (hPlus ? WHITE : (Color){120, 150, 120, 255});
		DrawFilledRectRounded((float)plusX, (float)ctrlY,
		                      (float)ctrlBtnS, (float)ctrlBtnS, r, pFill);
		DrawRectRoundedLines((float)plusX, (float)ctrlY,
		                     (float)ctrlBtnS, (float)ctrlBtnS, r, pLine);
		Color pText = (!shotsActive) ? (Color){80, 80, 80, 255}
		             : (hPlus ? WHITE : (Color){180, 200, 180, 255});
		int ptw = MeasureTextStr("+", ctrlFontH);
		DrawTextStr("+", (float)(plusX + (ctrlBtnS - ptw)/2),
		            (float)(ctrlY + 1), ctrlFontH, pText);

		bool hInf = PointInRect(g->aimx, g->aimy, infX, ctrlY, ctrlBtnS + 6, ctrlBtnS);
		Color iFill = g->customShotsInfinite
		    ? (hInf ? (Color){80, 80, 140, 255} : (Color){50, 50, 100, 255})
		    : (hInf ? (Color){50, 50, 50, 255}  : (Color){30, 30, 30, 255});
		Color iLine = g->customShotsInfinite
		    ? (hInf ? WHITE : (Color){150, 150, 220, 255})
		    : (hInf ? (Color){180, 180, 180, 255} : (Color){100, 100, 100, 255});
		DrawFilledRectRounded((float)infX, (float)ctrlY,
		                      (float)(ctrlBtnS + 6), (float)ctrlBtnS, r, iFill);
		DrawRectRoundedLines((float)infX, (float)ctrlY,
		                     (float)(ctrlBtnS + 6), (float)ctrlBtnS, r, iLine);
		Color iText = g->customShotsInfinite
		    ? (hInf ? WHITE : (Color){200, 200, 255, 255})
		    : (hInf ? WHITE : (Color){150, 150, 150, 255});
		int iw = MeasureTextStr("inf", ctrlFontH - 2);
		DrawTextStr("inf", (float)(infX + (ctrlBtnS + 6 - iw)/2),
		            (float)(ctrlY + 2), ctrlFontH - 2, iText);
	}

	// ── START GAME / BACK buttons ──
	int actFontH = ctrlFontH + 2;
	int startBtnW = g->winWidth * 30 / 100;
	if (startBtnW < 120) startBtnW = 120;
	int startBtnH = g->winHeight * 8 / 100;
	if (startBtnH < 32) startBtnH = 32;
	int startBtnX = g->centerX - startBtnW / 2;
	int startBtnY = g->winHeight * 65 / 100;
	int backBtnY  = startBtnY + startBtnH + 8;

	bool hoverStart = PointInRect(g->aimx, g->aimy, startBtnX, startBtnY, startBtnW, startBtnH);
	bool hoverBack  = PointInRect(g->aimx, g->aimy, startBtnX, backBtnY,  startBtnW, startBtnH);
	drawButton(startBtnX, startBtnY, startBtnW, startBtnH, "START GAME", actFontH, hoverStart);
	drawButton(startBtnX, backBtnY,  startBtnW, startBtnH, "BACK",       actFontH, hoverBack);
}

void RenderPlaying(GameContext* g)
{
	drawSpiralGuide(g);
	drawBallList(g->head);

	if (!g->ballMoving && g->aiming) {
		DrawWideLine(g->cball.x, g->cball.y,
		             (float)g->aimx, (float)g->aimy, WHITE);
	}

	if (g->ballMoving)
		drawColBall(&g->cball, g->cball.x, g->cball.y);
	else
		drawColBall(&g->cball, (float)g->centerX, (float)g->centerY);
}

void RenderClearing(GameContext* g)
{
	// ── Phase 0: 球链沿螺旋线依次收缩消除 ──
	if (g->clearPhase == 0) {
		drawSpiralGuide(g);

		// 遍历所有球，各自按缓动进度沿螺旋线绘制
		const double k = (g->R_OUTER - g->R_INNER) / TOTAL_THETA;
		double thetaMax = g->R_OUTER / k;
		Node* p = g->head->next;
		while (p != NULL) {
			float totalT = spiralDistToCenter(g, &p->data)
			    / (1.5f * (float)BALLRADIUS * 60.0f);
			if (totalT < 0.15f) totalT = 0.15f;
			float t = p->data.animTimer / totalT;
			if (t > 1.0f) t = 1.0f;
			float progress = easeBallProgress(t);

			double theta = (double)p->data.theta
			    + (thetaMax - (double)p->data.theta) * (double)progress;
			double r = g->R_OUTER - k * theta;
			if (r < 0.0) r = 0.0;
			float sx = (float)(g->centerX + r * cos(theta));
			float sy = (float)(g->centerY + r * sin(theta));
			DrawFilledCircle(sx, sy, (float)BALLRADIUS,
			                 ballColorTable[(int)p->data.c]);
			p = p->next;
		}

		// 绘制中央球
		drawColBall(&g->cball, (float)g->centerX, (float)g->centerY);

		// popup 更新 + 绘制
		for (int i = 0; i < g->popupCount; ) {
			g->popups[i].y -= BALLRADIUS * 0.06f;
			g->popups[i].life--;
			if (g->popups[i].life <= 0)
				g->popups[i] = g->popups[--g->popupCount];
			else
				i++;
		}
		int popupFontH = BALLRADIUS * 2;
		if (popupFontH < 10) popupFontH = 10;
		for (int i = 0; i < g->popupCount; i++) {
			int brightness = g->popups[i].life * 255 / 30;
			char buf[16];
			if (g->popups[i].score >= 0)
				sprintf(buf, "+%d", g->popups[i].score);
			else
				sprintf(buf, "%d", g->popups[i].score);
			int tw = MeasureTextStr(buf, popupFontH);
			Color pc = (g->popups[i].score >= 0)
			    ? (Color){(unsigned char)brightness,
			             (unsigned char)brightness, 0, 255}
			    : (Color){(unsigned char)brightness, 0, 0, 255};
			DrawTextStr(buf,
			            g->popups[i].x - (float)tw / 2,
			            g->popups[i].y,
			            popupFontH, pc);
		}

		// 右上角 Score
		int scoreFontH = BALLRADIUS * 3;
		if (scoreFontH < 14) scoreFontH = 14;
		char scoreStr[32];
		sprintf(scoreStr, "Score: %d", g->totalScore);
		int sw = MeasureTextStr(scoreStr, scoreFontH);
		DrawTextStr(scoreStr, (float)(g->winWidth - sw - 10), 10.0f,
		            scoreFontH, (Color){255, 255, 200, 255});
		return;
	}

	// ── Phase 1: 百叶窗淡出 ──
	if (g->clearPhase == 1) {
		drawSpiralGuide(g);
		drawColBall(&g->cball, (float)g->centerX, (float)g->centerY);

		// 右上角 Score
		int scoreFontH = BALLRADIUS * 3;
		if (scoreFontH < 14) scoreFontH = 14;
		char scoreStr[32];
		sprintf(scoreStr, "Score: %d", g->totalScore);
		int sw = MeasureTextStr(scoreStr, scoreFontH);
		DrawTextStr(scoreStr, (float)(g->winWidth - sw - 10), 10.0f,
		            scoreFontH, (Color){255, 255, 200, 255});

		// 百叶窗：多条水平黑带从上到下依次扩张
		int numBlinds = 10;
		float blindH = (float)g->winHeight / (float)numBlinds;
		float blindProgress = g->clearTimer / 0.8f;
		if (blindProgress > 1.0f) blindProgress = 1.0f;
		for (int i = 0; i < numBlinds; i++) {
			float bandP = (blindProgress * (numBlinds + 1) - i);
			if (bandP < 0.0f) bandP = 0.0f;
			if (bandP > 1.0f) bandP = 1.0f;
			DrawRectangle(0.0f, i * blindH,
			              (float)g->winWidth, blindH * bandP, BLACK);
		}
		return;
	}

	// ── Phase 2: Score 飞入中央 ──
	{
		float t = g->clearTimer;
		float s = t / 1.0f;
		if (s > 1.0f) s = 1.0f;

		int cornerFontH = BALLRADIUS * 3;
		if (cornerFontH < 14) cornerFontH = 14;
		int centerFontH = g->winHeight / 6;
		int maxFontH = g->winWidth / 5;
		if (centerFontH > maxFontH) centerFontH = maxFontH;
		if (centerFontH < 24) centerFontH = 24;
		int curFontH = cornerFontH + (int)((centerFontH - cornerFontH) * s);

		char scoreStr[32];
		sprintf(scoreStr, "Score: %d", g->totalScore);
		int tw = MeasureTextStr(scoreStr, curFontH);

		int startX = g->winWidth - MeasureTextStr(scoreStr, cornerFontH) - 10;
		if (startX < 0) startX = 0;
		int startY = 10;
		int endX = g->centerX - tw / 2;
		int endY = g->centerY - curFontH / 2;

		int curX = startX + (int)((endX - startX) * s);
		int curY = startY + (int)((endY - startY) * s);
		DrawTextStr(scoreStr, (float)curX, (float)curY,
		            curFontH, (Color){255, 255, 200, 255});
		return;
	}
}

void RenderSettlement(const GameContext* g)
{
	int titleFontH = g->winHeight / 12;
	if (titleFontH < 18) titleFontH = 18;
	if (g->gameWon) {
		const char* ttl = "CLEAR!";
		int ttw = MeasureTextStr(ttl, titleFontH);
		DrawTextStr(ttl, (float)(g->centerX - ttw / 2),
		            (float)(g->winHeight / 8), titleFontH,
		            (Color){100, 255, 100, 255});
	} else {
		const char* ttl = "GAME OVER";
		int ttw = MeasureTextStr(ttl, titleFontH);
		DrawTextStr(ttl, (float)(g->centerX - ttw / 2),
		            (float)(g->winHeight / 8), titleFontH,
		            (Color){255, 100, 100, 255});
	}

	int scoreFontH = g->winHeight / 6;
	if (scoreFontH < 24) scoreFontH = 24;
	char scoreStr[32];
	sprintf(scoreStr, "Score: %d", g->totalScore);
	int sw = MeasureTextStr(scoreStr, scoreFontH);
	DrawTextStr(scoreStr, (float)(g->centerX - sw / 2),
	            (float)(g->centerY - scoreFontH), scoreFontH,
	            (Color){255, 255, 200, 255});

	int btnW = g->winWidth * 3 / 10;
	if (btnW < 120) btnW = 120;
	int btnH = g->winHeight * 8 / 100;
	if (btnH < 36) btnH = 36;
	int btnFontH = btnH / 2;
	int btnSpacing = btnH / 2;
	int btnX = g->centerX - btnW / 2;
	int tryAgainY = g->centerY + g->winHeight / 6;
	int exitY = tryAgainY + btnH + btnSpacing;

	bool hoverTryAgain = PointInRect(g->aimx, g->aimy, btnX, tryAgainY, btnW, btnH);
	bool hoverExit     = PointInRect(g->aimx, g->aimy, btnX, exitY, btnW, btnH);
	drawButton(btnX, tryAgainY, btnW, btnH, "TRY AGAIN", btnFontH, hoverTryAgain);
	drawButton(btnX, exitY,     btnW, btnH, "EXIT",      btnFontH, hoverExit);
}

// ═══════════════════════════════════════════════════════
//  Playing HUD (popups + score + shots, post-render)
// ═══════════════════════════════════════════════════════

void RenderPlayingHUD(GameContext* g)
{
	// Update + draw popups
	for (int i = 0; i < g->popupCount; ) {
		g->popups[i].y -= BALLRADIUS * 0.06f;
		g->popups[i].life--;
		if (g->popups[i].life <= 0)
			g->popups[i] = g->popups[--g->popupCount];
		else
			i++;
	}
	int popupFontH = BALLRADIUS * 2;
	if (popupFontH < 10) popupFontH = 10;
	for (int i = 0; i < g->popupCount; i++) {
		int brightness = g->popups[i].life * 255 / 30;
		char buf[16];
		if (g->popups[i].score >= 0)
			sprintf(buf, "+%d", g->popups[i].score);
		else
			sprintf(buf, "%d", g->popups[i].score);
		int tw = MeasureTextStr(buf, popupFontH);
		Color pc = (g->popups[i].score >= 0)
		    ? (Color){(unsigned char)brightness,
		             (unsigned char)brightness, 0, 255}
		    : (Color){(unsigned char)brightness, 0, 0, 255};
		DrawTextStr(buf,
		            g->popups[i].x - (float)tw / 2,
		            g->popups[i].y,
		            popupFontH, pc);
	}

	// Score in top-right corner
	int scoreFontH = BALLRADIUS * 3;
	if (scoreFontH < 14) scoreFontH = 14;
	char scoreStr[32];
	sprintf(scoreStr, "Score: %d", g->totalScore);
	int sw = MeasureTextStr(scoreStr, scoreFontH);
	DrawTextStr(scoreStr, (float)(g->winWidth - sw - 10), 10.0f,
	            scoreFontH, (Color){255, 255, 200, 255});

	// Remaining shots
	int shotFontH = BALLRADIUS * 2;
	if (shotFontH < 10) shotFontH = 10;
	char shotStr[32];
	Color shotColor;
	if (g->remainingShots < 0) {
		shotColor = (Color){150, 255, 150, 255};
		sprintf(shotStr, "Shots: inf");
	} else if (g->remainingShots <= 5) {
		shotColor = (Color){255, 100, 100, 255};
		sprintf(shotStr, "Shots: %d", g->remainingShots);
	} else {
		shotColor = (Color){200, 200, 200, 255};
		sprintf(shotStr, "Shots: %d", g->remainingShots);
	}
	int ssw = MeasureTextStr(shotStr, shotFontH);
	DrawTextStr(shotStr, (float)(g->winWidth - ssw - 10),
	            10.0f + scoreFontH + 2, shotFontH, shotColor);
	// ── EXIT 按钮 ──
	int exitW = BALLRADIUS * 6;
	if (exitW < 60) exitW = 60;
	int exitH = BALLRADIUS * 5 / 2;
	if (exitH < 24) exitH = 24;
	int exitX = 10, exitY = 10;
	int exitFontH = exitH * 55 / 100;
	bool hoverExit = PointInRect(g->aimx, g->aimy, exitX, exitY, exitW, exitH);
	drawButton(exitX, exitY, exitW, exitH, "EXIT", exitFontH, hoverExit);
}
