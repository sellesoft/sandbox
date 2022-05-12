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

typedef u32 TextureBG; enum{
	TextureBG_Dirt,
	TextureBG_Surface,
	TextureBG_Sky,
	TextureBG_Trench,
	TextureBG_Fort,
	TextureBG_Tunnel,
};

typedef u32 TextureFG; enum{
	TextureFG_None,
	TextureFG_BritishPlayer,
	TextureFG_GermanPlayer,
	TextureFG_Pillar,
	TextureFG_Explosive,
	TextureFG_Sandbags,
	TextureFG_Ladder,
};

struct Tile{
	TextureBG fg;
	TextureFG bg;
    b32 bomb;
};

struct Player{
    vec2 pos;
    u32 health;
    u32 bombs;
};

struct NetInfo{
    u8 magic[4] = {'T','U','N','L'};
	Action move;
    vec2 pos;
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
u32 board_width;
u32 board_height;
u32 board_area;

#define LinearRow(pos) ((pos)/board_width)
#define LinearCol(pos) ((pos)%board_width)
#define ToLinear(row,col) ((board_width*(row))+(col))
#define TileAt(row,col) board[ToLinear(row,col)]
#define TileAtLinear(pos) board[pos]

u32 turn_count;
u32 player_idx;

Player player0;
Player player1;

#endif //TUNLLER_TYPES_H