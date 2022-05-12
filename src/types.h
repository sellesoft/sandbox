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

#endif //TUNLLER_TYPES_H