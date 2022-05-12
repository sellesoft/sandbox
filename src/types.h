#pragma once
#ifndef TUNLLER_TYPES_H
#define TUNLLER_TYPES_H
#include "kigu/common.h"

typedef u32 Action; enum{
    Action_None,
    Action_MoveUp,
    Action_MoveDown,
    Action_MoveRight,
    Action_MoveLeft,
	Action_DigUp,
    Action_DigDown,
    Action_DigRight,
    Action_DigLeft,
	Action_PlaceBomb,
	Action_DetonateBomb,
	Action_PlaceLadder,
    Action_QuitGame,
	Action_COUNT
};

str8 ActionStrs[Action_COUNT] ={
    str8l("None"),
	str8l("MoveUp"),
	str8l("MoveDown"),
	str8l("MoveRight"),
	str8l("MoveLeft"),
	str8l("DigUp"),
	str8l("DigDown"),
	str8l("DigRight"),
	str8l("DigLeft"),
	str8l("PlaceBomb"),
	str8l("DetonateBomb"),
	str8l("PlaceLadder"),
	str8l("QuitGame"),
};

typedef u32 TileBG; enum{
	TileBG_Dirt,
	TileBG_Surface,
	TileBG_Sky,
	TileBG_Trench,
	TileBG_Fort,
	TileBG_Tunnel,
};

typedef u32 TileFG; enum{
	TileFG_None          = 0,
	
	TileFG_Ladder        = (1 << 0),
	TileFG_Pillar        = (1 << 1),
	
	TileFG_BritishBomb   = (1 << 2),
	TileFG_GermanBomb    = (1 << 3),
	TileFG_BombWire      = (1 << 4),
	
	TileFG_BritishPlayer = (1 << 5),
	TileFG_GermanPlayer  = (1 << 6),
};

struct Tile{
	TileBG bg;
	TileFG fg;
};

struct Player{
    s32 x, y;
    u32 bombs;
};

struct NetInfo{
    u8 magic[4] = {'T','U','N','L'};
	Action move;
    s32 x, y;
    u32 uid;
};

#define CheckMagic(var)     \
(                         \
var.magic[0] == 'T' &&  \
var.magic[1] == 'U' &&  \
var.magic[2] == 'N' &&  \
var.magic[3] == 'L'     \
)


//~////////////////////////////////////////////////////////////////////////////////////////////////
//// @vars
Tile* board;
s32 board_width;
s32 board_height;
s32 board_area;

#define LinearRow(pos) ((pos)/board_width)
#define LinearCol(pos) ((pos)%board_width)
#define ToLinear(x,y) ((board_width*(y))+(x))
#define TileAt(x,y) board[ToLinear(x,y)]
#define TileAtLinear(pos) board[pos]

u32 turn_count;
u32 player_idx;

Player player0;
Player player1;

#endif //TUNLLER_TYPES_H