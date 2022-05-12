#pragma once
#ifndef TUNLLER_TYPES_H
#define TUNLLER_TYPES_H
#include "kigu/common.h"

enum MoveType_{
    Move_None,
    Move_Up,
    Move_Down,
    Move_Right,   
    Move_Left,   
    Move_QuitGame,
};typedef u32 MoveType;

typedef u32 TextureBG; enum{
	TextureBG_Dirt,
	TextureBG_Surface,
	TextureBG_Sky,
	TextureBG_Trench,
	TextureBG_Fort,
	TextureBG_Tunnel,
};

typedef u32 TextureFG; enum{
	TextureFG_BritishPlayer,
	TextureFG_GermanPlayer,
	TextureFG_Pillar,
	TextureFG_Explosive,
	TextureFG_Sandbags,
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
    MoveType move;
    vec2 pos;
    u32 uid;
};

Tile* board;
u32 board_width;
u32 board_height;

u32 turn_count;

#endif //TUNLLER_TYPES_H