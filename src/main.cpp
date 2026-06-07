#include "game.h"
#include "render.h"

int main()
{
	GameContext g;
	GameInit(&g);

	RenderInit(g.winWidth, g.winHeight, "ZUMA");
	recomputeDimensions(&g);

	while (!RenderShouldClose())
	{
		HandleResize(&g);
		g.counter++;

		// ── Input ──────────────────────────────────
		switch (g.state) {
		case MENU:       InputMenu(&g);       break;
		case CUSTOMIZE:  InputCustomize(&g);   break;
		case PLAYING:    InputPlaying(&g);     break;
		case SETTLEMENT: InputSettlement(&g);  break;
		default: break;
		}

		// ── Update ─────────────────────────────────
		if (g.state == PLAYING)
			UpdatePlaying(&g);

		// ── Render ─────────────────────────────────
		BeginRender(BLACK);
		switch (g.state) {
		case MENU:       RenderMenu(&g);       break;
		case CUSTOMIZE:  RenderCustomize(&g);   break;
		case PLAYING:    RenderPlaying(&g);     break;
		case CLEARING:   RenderClearing(&g);    break;
		case SETTLEMENT: RenderSettlement(&g);  break;
		}
		if (g.state == PLAYING)
			RenderPlayingHUD(&g);
		EndRender();
	}

	if (g.head != NULL)
		DestroyList(g.head);
	RenderClose();
	return 0;
}
