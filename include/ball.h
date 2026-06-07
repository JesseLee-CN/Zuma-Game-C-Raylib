
#pragma once

#ifndef _BALL_H_
#define _BALL_H_

extern int BALLRADIUS;

enum BallColor {
	COLOR_BLUE = 0,
	COLOR_GREEN,
	COLOR_RED,
	COLOR_YELLOW,
	COLOR_MAGENTA,
	COLOR_BROWN,
	COLOR_COUNT = 6
};

struct ball
{
	float x;
	float y;
	float theta;
	float animTimer;
	BallColor c;
};


#endif
