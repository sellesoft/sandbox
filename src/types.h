#pragma once
#ifndef TUNLLER_TYPES_H
#define TUNLLER_TYPES_H
#include "kigu/common.h"

typedef u32 Message; enum{
    Message_None,
    
    Message_HostGame, //message broadcast by client hosting game
    Message_JoinGame, //message broadcast in response to HostGame
    Message_QuitGame, //message broadcast by either client to end game
    Message_AcknowledgeMessage, //message broadcast by either client to indiciate that the last message was recieved
	
    Message_MoveUp,
    Message_MoveDown,
    Message_MoveRight,
    Message_MoveLeft,
	
	Message_DigUp,
    Message_DigDown,
    Message_DigRight,
    Message_DigLeft,
	
	Message_BuildLadder,
	Message_BuildPillar,
	
	Message_PlaceBomb,
	Message_DetonateBomb,
	
	Message_COUNT,
	
    Message_MOVES_START = Message_MoveUp,
    Message_MOVES_END = Message_DetonateBomb,
};

str8 MessageStrings[Message_COUNT] ={
    str8l("None"),
	
	str8l("HostGame"),
	str8l("JoinGame"),
	str8l("QuitGame"),
	str8l("AcknowledgeMessage"),
	
	str8l("MoveUp"),
	str8l("MoveDown"),
	str8l("MoveRight"),
	str8l("MoveLeft"),
	
	str8l("DigUp"),
	str8l("DigDown"),
	str8l("DigRight"),
	str8l("DigLeft"),
	
	str8l("BuildLadder"),
	str8l("BuildPillar"),
	
	str8l("PlaceBomb"),
	str8l("DetonateBomb"),
};

typedef u32 TileBG; enum{
	TileBG_Dirt,
	TileBG_Tunnel,
	//TileBG_Trench,
	TileBG_BritishBase,
	TileBG_GermanBase,
	TileBG_Sky,
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
	u32 player_flag;
	u32 bomb_flag;
	array<u32> placed_bombs;
};

struct NetInfo{
    u32 magic = PackU32('T', 'U', 'N', 'L');
	Message message;
    s32 x, y;
    u32 uid;
};

#define CheckMagic(var) (var.magic == PackU32('T','U','N','L'))


//~////////////////////////////////////////////////////////////////////////////////////////////////
//// @vars
b32 game_active = 0;

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
NetInfo turn_info;

Player player0;
Player player1;
u32 player_idx;

Message last_action;
u32 last_action_x;
u32 last_action_y;

u64 net_port = 24465;

#endif //TUNLLER_TYPES_H