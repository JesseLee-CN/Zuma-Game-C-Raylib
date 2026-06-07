#pragma once

#include "ball.h"
#include "LinkList.h"

#define MAX_POPUPS 8

enum GameState { MENU, CUSTOMIZE, PLAYING, CLEARING, SETTLEMENT };

struct ScorePopup {
	float x, y;
	int score;
	int life;
};

struct GameContext {
	// Window / geometry
	int winWidth;
	int winHeight;
	int centerX;
	int centerY;
	double R_OUTER;
	double R_INNER;

	// State machine
	GameState state;
	bool gameWon;
	float clearTimer;
	int clearPhase;
	float clearBallProgress;
	float clearActiveTheta;

	// Difficulty config
	int startBalls;
	int startShots;

	// Customize page
	int customBalls;
	int customShots;
	bool customShotsInfinite;
	int customHoldBtn;
	float customHoldTime;
	int customBatchCount;

	// Playing state
	Node* head;
	ball cball;
	bool ballMoving;
	bool aiming;
	int aimx;
	int aimy;
	float vx;
	float vy;
	int counter;
	int totalScore;
	ScorePopup popups[MAX_POPUPS];
	int popupCount;
	int remainingShots;
};

// ── Init & utilities ──────────────────────────────────
void GameInit(GameContext* g);
void HandleResize(GameContext* g);
void recomputeDimensions(GameContext* g);

// ── Ball helpers ──────────────────────────────────────
void initBallList(Node* head, int count);
void updateBallPos(const GameContext* g, Node* head);
bool collisionDetection(Node* head, ball b, int* id);
void drawBallList(Node* head);
void drawColBall(ball* b, float x, float y);

// ── Rendering helpers ─────────────────────────────────
void drawSpiralGuide(const GameContext* g);
void drawButton(int x, int y, int w, int h,
                const char* text, int fontH, bool hovered);

// ── State input handlers ──────────────────────────────
void InputMenu(GameContext* g);
void InputCustomize(GameContext* g);
void InputPlaying(GameContext* g);
void InputSettlement(GameContext* g);

// ── Game update ────────────────────────────────────────
void UpdatePlaying(GameContext* g);
void UpdateClearing(GameContext* g);

// ── State render handlers ─────────────────────────────
void RenderMenu(const GameContext* g);
void RenderCustomize(const GameContext* g);
void RenderPlaying(GameContext* g);
void RenderClearing(GameContext* g);
void RenderSettlement(const GameContext* g);
void RenderPlayingHUD(GameContext* g);
